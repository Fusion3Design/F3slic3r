#ifndef slic3r_PresetArchiveDatabase_hpp_
#define slic3r_PresetArchiveDatabase_hpp_

#include "Event.hpp"

#include <string>
#include <vector>
#include <memory>

class boost::filesystem::path;

namespace Slic3r {
class AppConfig;
namespace GUI {

struct ArchiveRepositorySyncData
{
	std::string json;
	bool force_updater;
};

wxDECLARE_EVENT(EVT_PRESET_ARCHIVE_DATABASE_SYNC_DONE, Event<ArchiveRepositorySyncData>);


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
		boost::filesystem::path local_path;
		bool        m_secret { false };
	};
	// Use std::move when calling constructor.
	ArchiveRepository(RepositoryManifest&& data) : m_data(std::move(data)) {}
	~ArchiveRepository() {}
	// Gets vendor_indices.zip to target_path
	virtual bool get_archive(const boost::filesystem::path& target_path) const = 0;
	// Gets file if repository_id arg matches m_id.
	// Should be used to get the most recent ini file and every missing resource. 
	virtual bool get_file(const std::string& source_subpath, const boost::filesystem::path& target_path, const std::string& repository_id) const = 0;
	// Gets file without id check - for not yet encountered vendors only!
	virtual bool get_ini_no_id(const std::string& source_subpath, const boost::filesystem::path& target_path) const = 0;
protected:
	RepositoryManifest m_data;
};

typedef std::vector<std::unique_ptr<const ArchiveRepository>> ArchiveRepositoryVector;

class OnlineArchiveRepository : public ArchiveRepository
{
public:
	OnlineArchiveRepository(RepositoryManifest&& data) : ArchiveRepository(std::move(data)) 
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
	LocalArchiveRepository(RepositoryManifest&& data) : ArchiveRepository(std::move(data)) {}
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
class PresetArchiveDatabase
{
public:
	PresetArchiveDatabase(AppConfig* app_config, wxEvtHandler* evt_handler);
	~PresetArchiveDatabase() {}
	
	const ArchiveRepositoryVector& get_archives() const { return m_archives; }
	void sync();
	void sync_blocking();
	void set_token(const std::string token) { m_token = token; }
	void set_local_archives(AppConfig* app_config);
	void set_archives(const std::string& json_body);
private:
	wxEvtHandler*               p_evt_handler;
	boost::filesystem::path	    m_unq_tmp_path;
	ArchiveRepositoryVector		m_archives;
	std::vector<std::string>	m_local_archive_adresses;
	std::string					m_token;
};

}} // Slic3r::GUI

#endif // PresetArchiveDatabase