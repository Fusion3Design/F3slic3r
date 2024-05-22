#include "PresetArchiveDatabase.hpp"

#include "slic3r/Utils/Http.hpp"
#include "slic3r/GUI/format.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/UserAccount.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/miniz_extension.hpp"

#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cctype>
#include <curl/curl.h>
#include <iostream>
#include <fstream>

namespace pt = boost::property_tree;
namespace fs = boost::filesystem;
namespace Slic3r {
namespace GUI {

static const char* TMP_EXTENSION = ".download";

namespace {
bool unzip_repository(const fs::path& source_path, const fs::path& target_path)
{
	mz_zip_archive archive;
	mz_zip_zero_struct(&archive);
	if (!open_zip_reader(&archive, source_path.string())) {
		BOOST_LOG_TRIVIAL(error) << "Couldn't open zipped Archive Repository. " << source_path;
		return false;
	}
	size_t num_files = mz_zip_reader_get_num_files(&archive);

	for (size_t i = 0; i < num_files; ++i) {
		mz_zip_archive_file_stat file_stat;
		if (!mz_zip_reader_file_stat(&archive, i, &file_stat)) {
			BOOST_LOG_TRIVIAL(error) << "Failed to get file stat for file #" << i << " in the zip archive. Ending Unzipping.";
			close_zip_reader(&archive);
			return false;
		}
		fs::path extracted_path = target_path / file_stat.m_filename;
		if (file_stat.m_is_directory) {
			// Create directory if it doesn't exist
			fs::create_directories(extracted_path);
			continue;
		}
		// Create parent directory if it doesn't exist
		fs::create_directories(extracted_path.parent_path());
		// Extract file
		if (!mz_zip_reader_extract_to_file(&archive, i, extracted_path.string().c_str(), 0)) {
			BOOST_LOG_TRIVIAL(error) << "Failed to extract file #" << i << " from the zip archive. Ending Unzipping.";
			close_zip_reader(&archive);
			return false;
		}
	}
	close_zip_reader(&archive);
	return true;
}

bool extract_repository_header(const pt::ptree& ptree, ArchiveRepository::RepositoryManifest& data)
{
	// mandatory atributes
	if (const auto name = ptree.get_optional<std::string>("name"); name){
		data.name = *name;
	} else {
		BOOST_LOG_TRIVIAL(error) << "Failed to find \"name\" parameter in repository manifest. Repository is invalid.";
		return false;
	}
	if (const auto id = ptree.get_optional<std::string>("id"); id) {
		data.id = *id;
	}
	else {
		BOOST_LOG_TRIVIAL(error) << "Failed to find \"id\" parameter in repository manifest. Repository is invalid.";
		return false;
	}
	if (const auto url = ptree.get_optional<std::string>("url"); url) {
		data.url = *url;
	}
	else {
		BOOST_LOG_TRIVIAL(error) << "Failed to find \"url\" parameter in repository manifest. Repository is invalid.";
		return false;
	}
	// optional atributes
	if (const auto index_url = ptree.get_optional<std::string>("index_url"); index_url) {
		data.index_url = *index_url;
	}
	if (const auto description = ptree.get_optional<std::string>("description"); description) {
		data.description = *description;
	}
	if (const auto visibility = ptree.get_optional<std::string>("visibility"); visibility) {
		data.visibility = *visibility;
	}
	return true;
}

void delete_path_recursive(const fs::path& path)
{
	try {
		if (fs::exists(path)) {
			for (fs::directory_iterator it(path); it != fs::directory_iterator(); ++it) {
				const fs::path subpath = it->path();
				if (fs::is_directory(subpath)) {
					delete_path_recursive(subpath);
				} else {
					fs::remove(subpath);
				}
			}
			fs::remove(path);
		}
	}
	catch (const std::exception& e) {
		BOOST_LOG_TRIVIAL(error) << "Failed to delete files at: " << path;
	}
}

bool extract_local_archive_repository(const std::string& uuid, const fs::path& zip_path, fs::path& unq_tmp_path, ArchiveRepository::RepositoryManifest& manifest_data)
{
	manifest_data.source_path = zip_path;
	// Path where data will be unzipped.
	manifest_data.tmp_path = unq_tmp_path / uuid;
	// Delete previous data before unzip.
	// We have unique path in temp set for whole run of slicer and in it folder for each repo. 
	delete_path_recursive(manifest_data.tmp_path);
	fs::create_directories(manifest_data.tmp_path);
	// Unzip repository zip to unique path in temp directory.
	if (!unzip_repository(zip_path, manifest_data.tmp_path)) {
		return false;
	}
	// Read the manifest file.
	fs::path manifest_path = manifest_data.tmp_path / "manifest.json";
	try
	{
		pt::ptree ptree;
		pt::read_json(manifest_path.string(), ptree);
		if (!extract_repository_header(ptree, manifest_data)) {
			BOOST_LOG_TRIVIAL(error) << "Failed to load repository: " << zip_path;
			return false;
		}
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to read repository manifest JSON " << manifest_path << ". reason: " << e.what();
		return false;
	}
	return true;
}

void deserialize_string(const std::string& opt, std::vector<std::string>& result)
{
	std::string val;
	for (size_t i = 0; i < opt.length(); i++) {
		if (std::isspace(opt[i])) {
			continue;
		}
		if (opt[i] != ';') {
			val += opt[i];
		}
		else {
			result.emplace_back(std::move(val));
		}
	}
	if (!val.empty()) {
		result.emplace_back(std::move(val));
	}
}

std::string escape_string(const std::string& unescaped)
{
	std::string ret_val;
	CURL* curl = curl_easy_init();
	if (curl) {
		char* decoded = curl_easy_escape(curl, unescaped.c_str(), unescaped.size());
		if (decoded) {
			ret_val = std::string(decoded);
			curl_free(decoded);
		}
		curl_easy_cleanup(curl);
	}
	return ret_val;
}
std::string escape_path_by_element(const std::string& path_string)
{
	const boost::filesystem::path path(path_string);
	std::string ret_val = escape_string(path.filename().string());
	boost::filesystem::path parent(path.parent_path());
	while (!parent.empty() && parent.string() != "/") // "/" check is for case "/file.gcode" was inserted. Then boost takes "/" as parent_path.
	{
		ret_val = escape_string(parent.filename().string()) + "/" + ret_val;
		parent = parent.parent_path();
	}
	return ret_val;
}

void add_authorization_header(Http& http)
{
    const std::string access_token = GUI::wxGetApp().plater()->get_user_account()->get_access_token();
    if (!access_token.empty()) {
        http.header("Authorization", "Bearer " + access_token);
    }
}

}

bool OnlineArchiveRepository::get_file_inner(const std::string& url, const fs::path& target_path) const
{

	bool res = false;
	fs::path tmp_path = target_path;
	tmp_path += format(".%1%%2%", get_current_pid(), TMP_EXTENSION);
	BOOST_LOG_TRIVIAL(info) << format("Get: `%1%`\n\t-> `%2%`\n\tvia tmp path `%3%`",
		url,
		target_path.string(),
		tmp_path.string());

	auto http = Http::get(url);
    add_authorization_header(http);
    http
		.timeout_max(30)
		.on_progress([](Http::Progress, bool& cancel) {
			//if (cancel) { cancel = true; }
		})
		.on_error([&](std::string body, std::string error, unsigned http_status) {
			BOOST_LOG_TRIVIAL(error) << format("Error getting: `%1%`: HTTP %2%, %3%",
				url,
				http_status,
				body);
		})
		.on_complete([&](std::string body, unsigned /* http_status */) {
			if (body.empty()) {
				return;
			}
			fs::fstream file(tmp_path, std::ios::out | std::ios::binary | std::ios::trunc);
			file.write(body.c_str(), body.size());
			file.close();
			fs::rename(tmp_path, target_path);
			res = true;
		})
		.perform_sync();	

	return res;
}

bool OnlineArchiveRepository::get_archive(const fs::path& target_path) const
{
	return get_file_inner(m_data.index_url.empty() ? m_data.url + "vendor_indices.zip" : m_data.index_url, target_path);
}

bool OnlineArchiveRepository::get_file(const std::string& source_subpath, const fs::path& target_path, const std::string& repository_id) const
{
	if (repository_id != m_data.id) {
		BOOST_LOG_TRIVIAL(error) << "Error getting file " << source_subpath << ". The repository_id was not matching.";
	    return false;
	}
	const std::string escaped_source_subpath = escape_path_by_element(source_subpath);
	return get_file_inner(m_data.url + escaped_source_subpath, target_path);
}

bool OnlineArchiveRepository::get_ini_no_id(const std::string& source_subpath, const fs::path& target_path) const
{
	const std::string escaped_source_subpath = escape_path_by_element(source_subpath);
	return get_file_inner(m_data.url + escaped_source_subpath, target_path);
}

bool LocalArchiveRepository::get_file_inner(const fs::path& source_path, const fs::path& target_path) const
{
	BOOST_LOG_TRIVIAL(debug) << format("Copying %1% to %2%", source_path, target_path);
	std::string error_message;
	CopyFileResult cfr = Slic3r::copy_file(source_path.string(), target_path.string(), error_message, false);
	if (cfr != CopyFileResult::SUCCESS) {
		BOOST_LOG_TRIVIAL(error) << "Copying of " << source_path << " to " << target_path << " has failed (" << cfr << "): " << error_message;
		// remove target file, even if it was there before
		if (fs::exists(target_path)) {
			boost::system::error_code ec;
			fs::remove(target_path, ec);
			if (ec) {
				BOOST_LOG_TRIVIAL(error) << format("Failed to delete file: %1%", ec.message());
			}
		}
		return false;
	}
	// Permissions should be copied from the source file by copy_file(). We are not sure about the source
	// permissions, let's rewrite them with 644.
	static constexpr const auto perms = fs::owner_read | fs::owner_write | fs::group_read | fs::others_read;
	fs::permissions(target_path, perms);

	return true;
}

bool LocalArchiveRepository::get_file(const std::string& source_subpath, const fs::path& target_path, const std::string& repository_id) const
{
	if (repository_id != m_data.id) {
		BOOST_LOG_TRIVIAL(error) << "Error getting file " << source_subpath << ". The repository_id was not matching.";
		return false;
	}
	return get_file_inner(m_data.tmp_path / source_subpath, target_path);
}
bool LocalArchiveRepository::get_ini_no_id(const std::string& source_subpath, const fs::path& target_path) const
{
	return get_file_inner(m_data.tmp_path / source_subpath, target_path);
}
bool LocalArchiveRepository::get_archive(const fs::path& target_path) const
{
	fs::path source_path = fs::path(m_data.tmp_path) / "vendor_indices.zip";
	return get_file_inner(std::move(source_path), target_path);
}


//-------------------------------------PresetArchiveDatabase-------------------------------------------------------------------------------------------------------------------------

PresetArchiveDatabase::PresetArchiveDatabase(AppConfig* app_config, wxEvtHandler* evt_handler)
	: p_evt_handler(evt_handler)
{
	// 
	boost::system::error_code ec;
	m_unq_tmp_path = fs::temp_directory_path() / fs::unique_path();
	fs::create_directories(m_unq_tmp_path, ec);
	assert(!ec);

	load_app_manifest_json();
}

bool PresetArchiveDatabase::set_selected_repositories(const std::vector<std::string>& selected_uuids, std::string& msg)
{
	// Check if some uuids leads to the same id (online vs local conflict)
	std::map<std::string, std::string> used_set;
	for (const std::string& uuid : selected_uuids) {
		std::string id;
		std::string name;
		for (const auto& archive : m_archive_repositories) {
			if (archive->get_uuid() == uuid) {
				id = archive->get_manifest().id;
				name = archive->get_manifest().name;
				break;
			}
		}
		assert(!id.empty());
		if (auto it = used_set.find(id); it != used_set.end()) {
			msg = GUI::format(_L("Cannot select two repositories with the same id: %1% and %2%"), it->second, name);
			return false;
		}
		used_set.emplace(id, name);
	}
	// deselect all first
	for (auto& pair : m_selected_repositories_uuid) {
		pair.second = false;
	}
	for (const std::string& uuid : selected_uuids) {
		m_selected_repositories_uuid[uuid] = true;
	}
	save_app_manifest_json();
	return true;
}

std::string PresetArchiveDatabase::add_local_archive(const boost::filesystem::path path, std::string& msg)
{
	if (auto it = std::find_if(m_archive_repositories.begin(), m_archive_repositories.end(), [path](const std::unique_ptr<const ArchiveRepository>& ptr) {
		return ptr->get_manifest().source_path == path;
		}); it != m_archive_repositories.end())
	{
		msg = GUI::format(_L("Failed to add local archive %1%. Path already used."), path);
		BOOST_LOG_TRIVIAL(error) << msg;
		return std::string();
	}
	std::string uuid = get_next_uuid();
	ArchiveRepository::RepositoryManifest header_data;
	if (!extract_local_archive_repository(uuid, path, m_unq_tmp_path, header_data)) {
		msg = GUI::format(_L("Failed to extract local archive %1%."), path);
		BOOST_LOG_TRIVIAL(error) << msg;
		return std::string();
	}
	// Solve if it can be set true first.
	m_selected_repositories_uuid[uuid] = false;
	m_archive_repositories.emplace_back(std::make_unique<LocalArchiveRepository>(uuid, std::move(header_data)));

	save_app_manifest_json();
	return uuid;
}
void PresetArchiveDatabase::remove_local_archive(const std::string& uuid)
{
	auto compare_repo = [uuid](const std::unique_ptr<const ArchiveRepository>& repo) {
		return repo->get_uuid() == uuid;
	};

	auto archives_it = std::find_if(m_archive_repositories.begin(), m_archive_repositories.end(), compare_repo);
	assert(archives_it != m_archive_repositories.end());
	std::string removed_uuid = archives_it->get()->get_uuid();
	m_archive_repositories.erase(archives_it);
	
	auto used_it = m_selected_repositories_uuid.find(removed_uuid);
	assert(used_it != m_selected_repositories_uuid.end());
	m_selected_repositories_uuid.erase(used_it);

	save_app_manifest_json();
}

void PresetArchiveDatabase::load_app_manifest_json()
{
	const fs::path path = get_stored_manifest_path();
	if (!fs::exists(path)) {
		copy_initial_manifest();
	}
	std::ifstream file(path.string());
	std::string data;
	if (file.is_open()) {
		std::string line;
		while (getline(file, line)) {
			data += line;
		}
		file.close();
	}
	else {
		assert(true);
		BOOST_LOG_TRIVIAL(error) << "Failed to read Archive Repository Manifest at " << path;
	}
	if (data.empty()) {
		return;
	}

	m_archive_repositories.clear();
	try
	{
		std::stringstream ss(data);
		pt::ptree ptree;
		pt::read_json(ss, ptree);
		for (const auto& subtree : ptree) {
			// if has tmp_path its local repo else its online repo (manifest is written in its zip, not in our json)
			if (const auto source_path = subtree.second.get_optional<std::string>("source_path"); source_path) {
				ArchiveRepository::RepositoryManifest manifest;
				std::string uuid = get_next_uuid();
				if (!extract_local_archive_repository(uuid, *source_path, m_unq_tmp_path, manifest)) {
					BOOST_LOG_TRIVIAL(error) << "Local archive repository not extracted: " << *source_path;
					continue;
				}
				if(const auto used = subtree.second.get_optional<bool>("selected"); used) {
					m_selected_repositories_uuid[uuid] = *used;
				} else {
					assert(true);
				}
				m_archive_repositories.emplace_back(std::make_unique<LocalArchiveRepository>(std::move(uuid), std::move(manifest)));
			
				continue;
			}
			// online repo
			ArchiveRepository::RepositoryManifest manifest;
			std::string uuid = get_next_uuid();
			if (!extract_repository_header(subtree.second, manifest)) {
				assert(true);
				BOOST_LOG_TRIVIAL(error) << "Failed to read one of repository headers.";
				continue;
			}
			auto search = m_selected_repositories_uuid.find(uuid);
			assert(search == m_selected_repositories_uuid.end());
			if (const auto used = subtree.second.get_optional<bool>("selected"); used) {
				m_selected_repositories_uuid[uuid] = *used;
			} else {
				assert(true);
			}
			m_archive_repositories.emplace_back(std::make_unique<OnlineArchiveRepository>(std::move(uuid), std::move(manifest)));
		}
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to read archives JSON. " << e.what();
	}
}

void PresetArchiveDatabase::copy_initial_manifest()
{
	const fs::path target_path = get_stored_manifest_path();
	const fs::path source_path = fs::path(resources_dir()) / "profiles" / "ArchiveRepositoryManifest.json";
	assert(fs::exists(source_path));
	std::string error_message;
	CopyFileResult cfr = Slic3r::copy_file(source_path.string(), target_path.string(), error_message, false);
	assert(cfr == CopyFileResult::SUCCESS);
	if (cfr != CopyFileResult::SUCCESS) {
		BOOST_LOG_TRIVIAL(error) << "Failed to copy ArchiveRepositoryManifest.json from resources.";
		return;
	}
	static constexpr const auto perms = fs::owner_read | fs::owner_write | fs::group_read | fs::others_read;
	fs::permissions(target_path, perms);
}

void PresetArchiveDatabase::save_app_manifest_json() const
{
	/*
	[{
		"name": "Production",
		"description": "Production repository",
		"visibility": null,
		"id": "prod",
		"url": "http://10.24.3.3:8001/v1/repos/prod",
		"index_url": "http://10.24.3.3:8001/v1/repos/prod/vendor_indices.zip"
	}, {
		"name": "Development",
		"description": "Production repository",
		"visibility": null,
		"id": "dev",
		"url": "http://10.24.3.3:8001/v1/repos/dev",
		"index_url": "http://10.24.3.3:8001/v1/repos/dev/vendor_indices.zip"
	}]
	*/
	std::string data = "[";

	for (const auto& archive : m_archive_repositories) {
		// local writes only source_path and "selected". Rest is read from zip on source_path.
		if (!archive->get_manifest().tmp_path.empty()) {
			const ArchiveRepository::RepositoryManifest& man = archive->get_manifest();
			std::string line = archive == m_archive_repositories.front() ? std::string() : ",";
			line += GUI::format(
				"{\"source_path\": \"%1%\","
				"\"selected\": %2%}"
				, man.source_path.generic_string(), is_selected(archive->get_uuid()) ? "1" : "0");
			data += line;
			continue;
		}
		// online repo writes whole manifest - in case of offline run, this info is load from here
		const ArchiveRepository::RepositoryManifest& man = archive->get_manifest();
		std::string line = archive == m_archive_repositories.front() ? std::string() : ",";
		line += GUI::format(
			"{\"name\": \"%1%\","
			"\"description\": \"%2%\","
			"\"visibility\": \"%3%\","
			"\"id\": \"%4%\","
			"\"url\": \"%5%\","
			"\"index_url\": \"%6%\","
			"\"selected\": %7%}"
			, man.name, man.description, man. visibility, man.id, man.url, man.index_url, is_selected(archive->get_uuid()) ? "1" : "0");
		data += line;
	}
	data += "]";

	std::string path = get_stored_manifest_path().string();
	std::ofstream file(path);
	if (file.is_open()) {
		file << data;
		file.close();
	} else {
		assert(true);
		BOOST_LOG_TRIVIAL(error) << "Failed to write Archive Repository Manifest to " << path;
	}
}

fs::path PresetArchiveDatabase::get_stored_manifest_path() const
{
	return (boost::filesystem::path(Slic3r::data_dir()) / "ArchiveRepositoryManifest.json").make_preferred();
}

bool PresetArchiveDatabase::is_selected(const std::string& uuid) const
{
	auto search = m_selected_repositories_uuid.find(uuid);
	assert(search != m_selected_repositories_uuid.end()); 
	return search->second;
}

void PresetArchiveDatabase::clear_online_repos()
{
	auto it = m_archive_repositories.begin();
	while (it != m_archive_repositories.end()) {
		if ((*it)->get_manifest().tmp_path.empty()) {
			it = m_archive_repositories.erase(it);
		} else {
			++it;
		}
	}
}

void PresetArchiveDatabase::read_server_manifest(const std::string& json_body)
{
	pt::ptree ptree;
	try
	{
		std::stringstream ss(json_body);
		pt::read_json(ss, ptree);
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to read archives JSON. " << e.what();

	}
	// Online repo manifests are in json_body. We already read local manifest and online manifest from last run.
	// Keep the local ones and replace the online ones but keep uuid for same id so the selected map is correct.
	// Solution: Create id - uuid translate table for online repos.
	std::map<std::string, std::string> id_to_uuid;
	for (const auto& repo_ptr : m_archive_repositories) {
		if (repo_ptr->get_manifest().source_path.empty()){
			id_to_uuid[repo_ptr->get_manifest().id] = repo_ptr->get_uuid();
		}
	}
	clear_online_repos();
	
	for (const auto& subtree : ptree) {
		ArchiveRepository::RepositoryManifest manifest;
		if (!extract_repository_header(subtree.second, manifest)) {
			assert(true);
			BOOST_LOG_TRIVIAL(error) << "Failed to read one of repository headers.";
			continue;
		}
		auto id_it = id_to_uuid.find(manifest.id);
		std::string uuid = (id_it == id_to_uuid.end() ? get_next_uuid() : id_it->second);
		// Set default used value to true - its a never before seen repository
		if (auto search = m_selected_repositories_uuid.find(uuid); search == m_selected_repositories_uuid.end()) {
			m_selected_repositories_uuid[uuid] = true;
		}
		m_archive_repositories.emplace_back(std::make_unique<OnlineArchiveRepository>(uuid, std::move(manifest)));
	}
	
	consolidate_selected_uuids_map();
	save_app_manifest_json();
}

bool PresetArchiveDatabase::is_selected_repository_by_uuid(const std::string& uuid) const
{
	auto selected_it = m_selected_repositories_uuid.find(uuid);
	assert(selected_it != m_selected_repositories_uuid.end());
	return selected_it->second;
}
bool PresetArchiveDatabase::is_selected_repository_by_id(const std::string& repo_id) const
{
	assert(!repo_id.empty());
	for (const auto& repo_ptr : m_archive_repositories) {
		if (repo_ptr->get_manifest().id == repo_id) {
			return true;
		}
	}
	return false;
}
void PresetArchiveDatabase::consolidate_selected_uuids_map()
{
	//std::vector<std::unique_ptr<const ArchiveRepository>> m_archive_repositories;
	//std::map<std::string, bool> m_selected_repositories_uuid;
	auto it = m_selected_repositories_uuid.begin();
	while (it != m_selected_repositories_uuid.end()) {
		bool found = false;
		for (const auto& repo_ptr : m_archive_repositories) {
			if (repo_ptr->get_uuid() == it->first) {
				found = true;
				break;	 
			}
		}
		if (!found) {
			it = m_selected_repositories_uuid.erase(it);
		} else {
			++it;
		}
	}
}

std::string PresetArchiveDatabase::get_next_uuid()
{
	boost::uuids::uuid uuid = m_uuid_generator();
	return boost::uuids::to_string(uuid);
}

namespace {
bool sync_inner(std::string& manifest)
{
	bool ret = false;
#ifdef SLIC3R_REPO_URL
    std::string url = SLIC3R_REPO_URL;
#else
    std::string url = "https://preset-repo-api-stage.prusa3d.com/v1/repos";
#endif
    auto http = Http::get(std::move(url));
    add_authorization_header(http);
    http
		.timeout_max(30)
		.on_error([&](std::string body, std::string error, unsigned http_status) {
			BOOST_LOG_TRIVIAL(error) << "Failed to get online archive repository manifests: "<< body << " ; " << error << " ; " << http_status;
			ret = false;
		})
		.on_complete([&](std::string body, unsigned /* http_status */) {
			manifest = body;
			ret = true;
		})
		.perform_sync();
	return ret;
}
}

void PresetArchiveDatabase::sync_blocking()
{
	BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " " << std::this_thread::get_id();
	std::string manifest;
	if (!sync_inner(manifest))
		return;
	read_server_manifest(std::move(manifest));
}

}} // Slic3r::GUI