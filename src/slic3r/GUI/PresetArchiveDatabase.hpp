#ifndef slic3r_PresetArchiveDatabase_hpp_
#define slic3r_PresetArchiveDatabase_hpp_

#include "Event.hpp"

#include <boost/uuid/uuid_generators.hpp>

#include <string>
#include <vector>
#include <memory>

namespace boost::filesystem {
class path;
}

namespace Slic3r {
class AppConfig;
namespace GUI {

struct ArchiveRepositoryGetFileArgs {
	boost::filesystem::path target_path;
	
	std::string repository_id;
};

class ArchiveRepository
{
public:
	struct RepositoryManifest {
		// mandatory
		std::string id;
		std::string name;
		std::string url;
		// optional
		std::string index_url;
		std::string description;
		std::string visibility;
		// not read from manifest json
		boost::filesystem::path tmp_path; // Where archive is unzziped. Created each app run. 
		boost::filesystem::path source_path; // Path given by user. Stored between app runs.
	};
	// Use std::move when calling constructor.
	ArchiveRepository(const std::string& uuid, RepositoryManifest&& data) : m_data(std::move(data)), m_uuid(uuid) {}
	virtual ~ArchiveRepository() {}
	// Gets vendor_indices.zip to target_path
	virtual bool get_archive(const boost::filesystem::path& target_path) const = 0;
	// Gets file if repository_id arg matches m_id.
	// Should be used to get the most recent ini file and every missing resource. 
	virtual bool get_file(const std::string& source_subpath, const boost::filesystem::path& target_path, const std::string& repository_id) const = 0;
	// Gets file without id check - for not yet encountered vendors only!
	virtual bool get_ini_no_id(const std::string& source_subpath, const boost::filesystem::path& target_path) const = 0;
	const RepositoryManifest& get_manifest() const { return m_data; }
	std::string get_uuid() const { return m_uuid; }
protected:
	RepositoryManifest m_data;
	std::string m_uuid;
};

class OnlineArchiveRepository : public ArchiveRepository
{
public:
	OnlineArchiveRepository(const std::string& uuid, RepositoryManifest&& data) : ArchiveRepository(uuid, std::move(data))
	{
		if (m_data.url.back() != '/') {
			m_data.url += "/";
		}
	}
	// Gets vendor_indices.zip to target_path.
	bool get_archive(const boost::filesystem::path& target_path) const override;
	// Gets file if repository_id arg matches m_id.
	// Should be used to get the most recent ini file and every missing resource. 
	bool get_file(const std::string& source_subpath, const boost::filesystem::path& target_path, const std::string& repository_id) const override;
	// Gets file without checking id.
	// Should be used only if no previous ini file exists.
	bool get_ini_no_id(const std::string& source_subpath, const boost::filesystem::path& target_path) const override;
private:
	bool get_file_inner(const std::string& url, const boost::filesystem::path& target_path) const;
};

class LocalArchiveRepository : public ArchiveRepository
{
public:
	LocalArchiveRepository(const std::string& uuid, RepositoryManifest&& data) : ArchiveRepository(uuid, std::move(data)) {}
	// Gets vendor_indices.zip to target_path.
	bool get_archive(const boost::filesystem::path& target_path) const override;
	// Gets file if repository_id arg matches m_id.
	// Should be used to get the most recent ini file and every missing resource. 
	bool get_file(const std::string& source_subpath, const boost::filesystem::path& target_path, const std::string& repository_id) const override;
	// Gets file without checking id.
	// Should be used only if no previous ini file exists.
	bool get_ini_no_id(const std::string& source_subpath, const boost::filesystem::path& target_path) const override;
private:
	bool get_file_inner(const boost::filesystem::path& source_path, const boost::filesystem::path& target_path) const;
};

typedef std::vector<std::unique_ptr<const ArchiveRepository>> ArchiveRepositoryVector;

class PresetArchiveDatabase
{
public:
	PresetArchiveDatabase(AppConfig* app_config, wxEvtHandler* evt_handler);
	~PresetArchiveDatabase() {}
	
	const ArchiveRepositoryVector& get_archive_repositories() const { return m_archive_repositories; }
	// Sync does download manifest for online repos.
	// returns true if preset update should be forced - if access token has changed, there might be new repositories or it is a first call
	//bool sync_blocking_with_token(const std::string& user_account_token);
	void set_access_token(const std::string& token) { m_token = token; }
	void sync_blocking();
	//void set_local_archives(AppConfig* app_config);
	void read_server_manifest(const std::string& json_body);
	const std::map<std::string, bool>& get_selected_repositories_uuid() const { assert(m_selected_repositories_uuid.size() == m_archive_repositories.size()); return m_selected_repositories_uuid; }
	bool set_selected_repositories(const std::vector<std::string>& used_uuids, std::string& msg);
	std::string add_local_archive(const boost::filesystem::path path, std::string& msg);
	void remove_local_archive(const std::string& uuid);
	// should be called only from main UI thread.
	void set_wizard_lock(bool lock);
private:
	void load_app_manifest_json();
	void save_app_manifest_json() const;
	void clear_online_repos();
	bool is_selected(const std::string& id) const;
	std::string get_stored_manifest_path() const;
	void consolidate_selected_uuids_map();
	std::string get_next_uuid();
	wxEvtHandler*					p_evt_handler;
	boost::filesystem::path			m_unq_tmp_path;
	ArchiveRepositoryVector			m_archive_repositories;
	std::map<std::string, bool>		m_selected_repositories_uuid;
	std::string						m_token;
	boost::uuids::random_generator	m_uuid_generator;
	bool							m_wizard_lock { false };
	bool							m_staged_sync { false };
};

}} // Slic3r::GUI

#endif // PresetArchiveDatabase