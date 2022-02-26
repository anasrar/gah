#pragma once
#include <string>
#include <vector>
#include <map>
#include <sqlite3.h>
#include <argparse/argparse.hpp>

namespace GAH
{
	enum EActions {
		NONE,
		LIST,
		SEARCH,
		CLIPBOARD,
		ADD,
		REMOVE,
		PUSH,
		PULL,
		CLONE,
	};

	std::string path_user_home();
	extern sqlite3 *db;
	extern std::string db_statement;
	extern sqlite3_stmt *db_statement_step;
	void db_connect();
	void db_disconnect();
	void db_add_token(
		const std::string &note,
		const std::string &username,
		const std::string &token,
		const std::string &expired
	);
	void db_get_username_and_token_from_id(
		const std::string &id,
		std::string &username,
		std::string &token
	);
	void db_get_token_list_by_filter(
		std::vector<std::vector<std::string>> &result,
		const std::string &filter,
		const std::string &query
	);

	void utility_getline_password(std::string &password, const std::string &username);

	argparse::ArgumentParser argparse_init();

	std::string github_construct_remote(const std::string &username, const std::string &token, const std::string &remote);
	void github_construct_command(std::string &comand_base, std::map<std::string, bool> &keys);
	void github_construct_command(std::string &comand_base, std::map<std::string, std::string> &keys);
	std::string github_get_dir_name_from_url(const std::string &url);

	static unsigned char aes_iv[16] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	};
	std::string encrypt_token(const std::string &token, const std::string &password);
	std::string decrypt_token(std::string &token, const std::string &password);

	bool git_check_command();
	bool git_check_local_repo();
	void git_check_command_and_local_repo();
	std::map<std::string, std::string> git_get_remotes();
	std::vector<std::string> git_get_branches();

	EActions str_to_actions(std::string str);
	void action_list_token(const int argc, const char *const argv[]);
	void action_search_token(const int argc, const char *const argv[]);
	void action_clipboard_token(const int argc, const char *const argv[]);
	void action_add_token(const int argc, const char *const argv[]);
	void action_remove_token(const int argc, const char *const argv[]);
	void action_git_push(const int argc, const char *const argv[]);
	void action_git_pull(const int argc, const char *const argv[]);
	void action_git_clone(const int argc, const char *const argv[]);
	void main_actions(const int argc, const char *const argv[]);
}
