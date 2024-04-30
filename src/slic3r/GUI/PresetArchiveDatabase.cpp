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
#include <cctype>
#include <curl/curl.h>

namespace pt = boost::property_tree;
namespace fs = boost::filesystem;
namespace Slic3r {
namespace GUI {

wxDEFINE_EVENT(EVT_PRESET_ARCHIVE_DATABASE_SYNC_DONE, Event<ArchiveRepositorySyncData>);

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
			mz_zip_reader_end(&archive);
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
			mz_zip_reader_end(&archive);
			return false;
		}
	}
	mz_zip_reader_end(&archive);
	return true;
}

bool extract_repository_header(const pt::ptree& ptree, ArchiveRepository::RepositoryManifest& data)
{
	// mandatory atributes
	if (const auto name = ptree.get_optional<std::string>("name"); name){
		data.name = *name;
	} else {
		BOOST_LOG_TRIVIAL(error) << "Failed to find \"name\" parameter in repository header. Repository is invalid.";
		return false;
	}
	if (const auto id = ptree.get_optional<std::string>("id"); id) {
		data.id = *id;
	}
	else {
		BOOST_LOG_TRIVIAL(error) << "Failed to find \"id\" parameter in repository header. Repository is invalid.";
		return false;
	}
	if (const auto url = ptree.get_optional<std::string>("url"); url) {
		data.url = *url;
	}
	else {
		BOOST_LOG_TRIVIAL(error) << "Failed to find \"url\" parameter in repository header. Repository is invalid.";
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
		data.m_secret = data.visibility.empty();
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

bool extract_local_archive_repository(const fs::path& zip_path, fs::path& unq_tmp_path, ArchiveRepository::RepositoryManifest& header_data)
{
	// Path where data will be unzipped.
	header_data.local_path = unq_tmp_path / zip_path.stem();
	// Delete previous data before unzip.
	// We have unique path in temp set for whole run of slicer and in it folder for each repo. 
	delete_path_recursive(header_data.local_path);
	fs::create_directories(header_data.local_path);
	// Unzip repository zip to unique path in temp directory.
	if (!unzip_repository(zip_path, header_data.local_path)) {
		return false;
	}
	// Read the header file.
	fs::path header_path = header_data.local_path / "header.json";
	try
	{
		pt::ptree ptree;
		pt::read_json(header_path.string(), ptree);
		if (!extract_repository_header(ptree, header_data)) {
			BOOST_LOG_TRIVIAL(error) << "Failed to load repository: " << zip_path;
			return false;
		}
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to read repository header JSON " << header_path << ". reason: " << e.what();
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
	return get_file_inner(m_data.local_path / source_subpath, target_path);
}
bool LocalArchiveRepository::get_ini_no_id(const std::string& source_subpath, const fs::path& target_path) const
{
	return get_file_inner(m_data.local_path / source_subpath, target_path);
}
bool LocalArchiveRepository::get_archive(const fs::path& target_path) const
{
	fs::path source_path = fs::path(m_data.local_path) / "vendor_indices.zip";
	return get_file_inner(std::move(source_path), target_path);
}

PresetArchiveDatabase::PresetArchiveDatabase(AppConfig* app_config, wxEvtHandler* evt_handler)
	: p_evt_handler(evt_handler)
{
	boost::system::error_code ec;
	m_unq_tmp_path = fs::temp_directory_path() / fs::unique_path();
	fs::create_directories(m_unq_tmp_path, ec);
	assert(!ec);

	set_local_archives(app_config);
}

void PresetArchiveDatabase::set_used_archives(const std::vector<std::string>& used_ids)
{
	m_used_archive_ids = used_ids;
}
void PresetArchiveDatabase::add_local_archive(const boost::filesystem::path path)
{
}
void PresetArchiveDatabase::remove_local_archive(const std::string& id)
{
}

void PresetArchiveDatabase::set_archives(const std::string& json_body)
{
	m_archives.clear();
	// Online repo headers are in json_body.
	try
	{
		std::stringstream ss(json_body);
		pt::ptree ptree;
		pt::read_json(ss, ptree);	
		for (const auto& subtree : ptree) {
			ArchiveRepository::RepositoryManifest header;
			if (extract_repository_header(subtree.second, header)) {
				m_archives.emplace_back(std::make_unique<OnlineArchiveRepository>(std::move(header)));
			} else {
				BOOST_LOG_TRIVIAL(error) << "Failed to read one of repository headers.";
			}
		}
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to read archives JSON. " << e.what();
	}
	// Local archives are stored as paths to zip.
	// PresetArchiveDatabase has its folder in temp, local archives are extracted there
	for (const std::string& archive_opt : m_local_archive_adresses) {
		ArchiveRepository::RepositoryManifest header_data;
		if (extract_local_archive_repository(archive_opt, m_unq_tmp_path, header_data)) {
			m_archives.emplace_back(std::make_unique<LocalArchiveRepository>(std::move(header_data)));
		}	
	}
	m_used_archive_ids.clear();
	m_used_archive_ids.reserve(m_archives.size());
	for (const auto& archive : m_archives) {
		m_used_archive_ids.emplace_back(archive->get_manifest().id);
	}
}

void PresetArchiveDatabase::set_local_archives(AppConfig* app_config)
{
	m_local_archive_adresses.clear();
	std::string opt = app_config->get("local_archives");
	deserialize_string(opt, m_local_archive_adresses);
}

namespace {
bool sync_inner(std::string& manifest)
{
	bool ret = false;
#ifdef SLIC3R_REPO_URL
    std::string url = SLIC3R_REPO_URL;
#else
    std::string url = "http://10.24.3.3:8001/v1/repos";
#endif
    auto http = Http::get(std::move(url));
    add_authorization_header(http);
    http
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


void PresetArchiveDatabase::sync()
{
	
	std::thread thread([this]() {
		std::string manifest;
		if (!sync_inner(manifest))
			return;
		// Force update when logged in (token not empty).
		wxQueueEvent(this->p_evt_handler, new Event<ArchiveRepositorySyncData>(EVT_PRESET_ARCHIVE_DATABASE_SYNC_DONE, {std::move(manifest), !m_token.empty()}));
	});
	thread.join();
}

void PresetArchiveDatabase::sync_blocking()
{
	std::string manifest;
	if (!sync_inner(manifest))
		return;
	set_archives(std::move(manifest));
}

}} // Slic3r::GUI