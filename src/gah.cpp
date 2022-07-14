#include "gah.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <map>
#include <algorithm>
#include <filesystem>
#include <regex>
#include <sqlite3.h>
#include <argparse/argparse.hpp>
#include <tabulate/tabulate.hpp>
#include <clip.h>
#include <picosha2.h>
#include <plusaes/plusaes.hpp>

#if defined(_WIN32) || defined(WIN32)
#define OS_WINDOWS
#endif

#ifdef OS_WINDOWS
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

std::string GAH::path_user_home()
{
	std::string result;
	#ifdef OS_WINDOWS
		result = reinterpret_cast<char*>(getenv("HOMEDRIVE"));
		result += reinterpret_cast<char*>(getenv("HOMEPATH"));
		result += "\\";
	#else
		result = reinterpret_cast<char*>(getenv("HOME"));
		result += "/";
	#endif
	return result;
}

sqlite3* GAH::db;
std::string GAH::db_statement;
sqlite3_stmt* GAH::db_statement_step;

void GAH::db_connect()
{
	if(sqlite3_open((GAH::path_user_home() + ".gahdb").c_str(), &GAH::db) != SQLITE_OK)
	{
		std::cout << "Can't open database: " << sqlite3_errmsg(GAH::db) << std::endl;
		exit(1);
	}

	GAH::db_statement = "CREATE TABLE IF NOT EXISTS token (id INTEGER PRIMARY KEY, note TEXT, username TEXT, token TEXT, expired TEXT, UNIQUE (note, username, token, expired) ON CONFLICT ABORT)";
	sqlite3_prepare(GAH::db, GAH::db_statement.c_str(), -1, &GAH::db_statement_step, NULL);
	sqlite3_step(GAH::db_statement_step);
	sqlite3_finalize(GAH::db_statement_step);
}

void GAH::db_disconnect()
{
	sqlite3_close(GAH::db);
	sqlite3_shutdown();
}

void GAH::db_add_token(const std::string &note, const std::string &username, const std::string &token, const std::string &expired)
{
	GAH::db_statement = "INSERT INTO token (note, username, token, expired) VALUES (?, ?, ?, ?)";
	sqlite3_prepare(GAH::db, GAH::db_statement.c_str(), -1, &GAH::db_statement_step, NULL);
	sqlite3_bind_text(GAH::db_statement_step, 1, note.c_str(), note.length(), SQLITE_TRANSIENT);
	sqlite3_bind_text(GAH::db_statement_step, 2, username.c_str(), username.length(), SQLITE_TRANSIENT);
	sqlite3_bind_text(GAH::db_statement_step, 3, token.c_str(), token.length(), SQLITE_TRANSIENT);
	sqlite3_bind_text(GAH::db_statement_step, 4, expired.c_str(), expired.length(), SQLITE_TRANSIENT);
	sqlite3_step(GAH::db_statement_step);
	sqlite3_finalize(GAH::db_statement_step);
}

void GAH::db_get_username_and_token_from_id(const std::string &id, std::string &username, std::string &token)
{
	GAH::db_statement = "SELECT username, token FROM token WHERE id = ? LIMIT 1";
	sqlite3_prepare(GAH::db, GAH::db_statement.c_str(), -1, &GAH::db_statement_step, NULL);

	try
	{
		sqlite3_bind_int(GAH::db_statement_step, 1, std::stoi(id));
	}
	catch(...)
	{
		std::cout << "id is not valid" << std::endl;
		exit(1);
	}

	sqlite3_step(GAH::db_statement_step);

	if (!sqlite3_column_text(GAH::db_statement_step, 0))
	{
		std::cout << "id not found on data base" << std::endl;
		exit(1);
	}

	std::vector<std::string> result{};
	for(int i = 0; i < 2; i++)
		result.push_back(std::string((char*)sqlite3_column_text(GAH::db_statement_step, i)));
	sqlite3_finalize(GAH::db_statement_step);

	username = result[0];
	token = result[1];
}

void GAH::db_get_token_list_by_filter(std::vector<std::vector<std::string>> &result, const std::string &filter, const std::string &query)
{
	std::map<std::string, std::string> filter_type{
		{"id", "id = ?"},
		{"note", "note LIKE '%'||?||'%'"},
		{"username", "username = ?"},
		{"expired", "expired LIKE '%'||?||'%'"},
	};
	bool is_using_filter = false;

	GAH::db_statement = "SELECT id, note, username, expired FROM token";
	if (filter.size() && query.size() && filter_type.find(filter) != filter_type.end())
	{
		GAH::db_statement += " WHERE " + filter_type[filter];
		is_using_filter = true;
	}
	sqlite3_prepare(GAH::db, GAH::db_statement.c_str(), -1, &GAH::db_statement_step, NULL);
	if (is_using_filter)
	{
		if (filter == "id")
		{
			try {
				sqlite3_bind_int(GAH::db_statement_step, 1, std::stoi(query));
			}
			catch(...) {
				std::cout << "id is not valid" << std::endl;
				exit(1);
			}
		}
		else
		{
			sqlite3_bind_text(GAH::db_statement_step, 1, query.c_str(), query.length(), SQLITE_TRANSIENT);
		}
	}
	sqlite3_step(GAH::db_statement_step);

	result = {
		{},
		{},
		{},
		{},
	};

	while(sqlite3_column_text(GAH::db_statement_step, 0))
	{
		for (int i = 0; i < 4; ++i)
			result[i].push_back(std::string((char*)sqlite3_column_text(GAH::db_statement_step, i)));
		sqlite3_step(GAH::db_statement_step);
	}
	sqlite3_finalize(GAH::db_statement_step);
}

void GAH::utility_getline_password(std::string &password, const std::string &username)
{
	std::cout << "insert password for " << username << ": ";

	#ifdef OS_WINDOWS
		HANDLE std_in_handle = GetStdHandle(STD_INPUT_HANDLE); 
		DWORD console_mode = 0;
		GetConsoleMode(std_in_handle, &console_mode);
		SetConsoleMode(std_in_handle, console_mode & (~ENABLE_ECHO_INPUT));
		std::getline(std::cin, password);
		SetConsoleMode(std_in_handle, console_mode);
	#else
		termios old_std_in_attr;
		tcgetattr(STDIN_FILENO, &old_std_in_attr);
		termios mode = old_std_in_attr;
		mode.c_lflag &= ~ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, &mode);
		std::getline(std::cin, password);
		tcsetattr(STDIN_FILENO, TCSANOW, &old_std_in_attr);
	#endif

	std::cout << std::endl;
}

argparse::ArgumentParser GAH::argparse_init()
{
	argparse::ArgumentParser program("gah", std::string(GAH_VERSION));

	program.add_argument("action")
		.required()
		.help("[list | search | clipboard | add | remove | push | pull | clone]");

	return program;
}

std::string GAH::github_construct_remote(const std::string &username, const std::string &token, const std::string &remote)
{
	std::string result("https://" + username + ":" + token + "@");
	result += std::regex_replace(remote, std::regex("^https?://"), "");
	return result;
}

void GAH::github_construct_command(std::string &comand_base, std::map<std::string, bool> &keys)
{
	for(const auto &[key, val]: keys)
		if (val)
			comand_base += " " + key;
}

void GAH::github_construct_command(std::string &comand_base, std::map<std::string, std::string> &keys)
{
	for(const auto &[key, val]: keys)
		if (val != "")
			comand_base += " " + key + " " + val;
}

std::string GAH::github_get_dir_name_from_url(const std::string &url)
{
	return std::string(std::regex_replace(url, std::regex("((git@|https://)([\\w\\.@]+)(/|:))([\\w,\\-,\\_]+)/([\\w,\\-,\\_]+)(.git){0,1}((/){0,1})"), "$6"));
}

std::string GAH::encrypt_token(const std::string &token, const std::string &password)
{
	std::vector<unsigned char> key(picosha2::k_digest_size);
	picosha2::hash256(password, key);

	const std::uint64_t encrypted_size = plusaes::get_padded_encrypted_size(token.size());
	std::vector<unsigned char> encrypted_token(encrypted_size);
	plusaes::encrypt_cbc((unsigned char*)token.data(), token.size(), &key[0], key.size(), &GAH::aes_iv, &encrypted_token[0], encrypted_token.size(), true);

	std::vector<unsigned char> encrypted_size_byte(sizeof(encrypted_size));
	memcpy(&encrypted_size_byte[0], reinterpret_cast<const unsigned char*>(&encrypted_size), sizeof(encrypted_size));

	encrypted_token.insert(encrypted_token.end(), encrypted_size_byte.begin(), encrypted_size_byte.end());
	std::string result;
	std::stringstream ss;
	for(unsigned char x: encrypted_token)
		ss << std::setfill('0') << std::setw(2) << std::hex << (0xFF & x);
	ss >> result;

	return result;
}

std::string GAH::decrypt_token(std::string &token, const std::string &password)
{
	std::vector<unsigned char> key(picosha2::k_digest_size);
	picosha2::hash256(password, key);

	// add space to hex token
	for (int i = 2; i < token.size(); i += 3)
		token.insert(i, std::string(" ").c_str());

	std::istringstream ss(token);
	std::vector<unsigned char> encrypted_token;
	unsigned int cc;
	while (ss >> std::hex >> cc)
		encrypted_token.push_back(cc);

	std::uint64_t encrypted_size;
	std::vector<unsigned char> encrypted_size_byte(encrypted_token.end() - sizeof(encrypted_size), encrypted_token.end());
	encrypted_size = *reinterpret_cast<const std::uint64_t*>(&encrypted_size_byte[0]);

	encrypted_token.erase(encrypted_token.end() - sizeof(encrypted_size), encrypted_token.end());

	unsigned long padded_size = 0;
	std::vector<unsigned char> decrypted_token(encrypted_size);

	plusaes::decrypt_cbc(&encrypted_token[0], encrypted_token.size(), &key[0], key.size(), &GAH::aes_iv, &decrypted_token[0], decrypted_token.size(), &padded_size);

	std::string result(std::string(decrypted_token.begin(), decrypted_token.end()).c_str());

	std::regex regex_flag("^GAH_FLAG_");
	if(!std::regex_search(result, regex_flag))
		throw std::invalid_argument("password wrong");
	result = std::regex_replace(result, regex_flag, "");

	return result;
}


bool GAH::git_check_command()
{
	const char *command;
	#if defined(OS_WINDOWS) && !defined(TARGET_GIT_BASH)
		command = "git --version > nul 2>&1";
	#elif defined(OS_WINDOWS) && defined(TARGET_GIT_BASH)
		return true;
	#else
		command = "git --version &> /dev/null";
	#endif
	return 0 == system(command);
}

std::filesystem::path GAH::git_path_local_repo;
bool GAH::git_check_local_repo()
{
	std::filesystem::path p = std::filesystem::current_path();
	int counter = 0;
	while (true) {
		if (std::filesystem::is_directory(p / ".git"))
		{
			GAH::git_path_local_repo = p;
			return true;
		}

		if (p == p.root_directory()) return false;
		p = p.parent_path();

		if (counter > 100)
		{
			return false;
		}
		counter++;
	}
	return false;
}

void GAH::git_check_command_and_local_repo()
{
	if (!GAH::git_check_command())
	{
		std::cout << "git command not found" << std::endl;
		exit(1);
	}
	else if (!GAH::git_check_local_repo())
	{
		std::cout << "git repository not found in this folder" << std::endl;
		exit(1);
	}
}

std::map<std::string, std::string> GAH::git_get_remotes()
{
	std::map<std::string, std::string> result{};
	std::ifstream file(GAH::git_path_local_repo / ".git/config");
	std::string config_ini((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	std::regex regex_token("remote \"(.+)\"\\]\n\turl = (.+)");
	std::smatch matches;

	while (std::regex_search(config_ini, matches, regex_token))
	{
		if (matches.size() > 2)
			result.insert({matches[1], matches[2]});
		config_ini = matches.suffix();
	}

	return result;
}

std::vector<std::string> GAH::git_get_branches()
{
	std::vector<std::string> result;
	for (const auto & iter : std::filesystem::directory_iterator(GAH::git_path_local_repo / ".git/refs/heads"))
		result.push_back(iter.path().filename().u8string());

	return result;
}

GAH::EActions GAH::str_to_actions(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c);});
	if (str == "list") return LIST;
	if (str == "search") return SEARCH;
	if (str == "clipboard") return CLIPBOARD;
	if (str == "add") return ADD;
	if (str == "remove") return REMOVE;
	if (str == "push") return PUSH;
	if (str == "pull") return PULL;
	if (str == "clone") return CLONE;
	return NONE;
}

void GAH::action_list_token(const int argc, const char *const argv[])
{
	argparse::ArgumentParser program = GAH::argparse_init();

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		std::cout << err.what() << std::endl;
		std::cout << program;
		exit(1);
	}

	std::vector<std::vector<std::string>> result;
	GAH::db_get_token_list_by_filter(result, std::string(""), std::string(""));

	tabulate::Table table_token;

	if (!result[0].size())
	{
		table_token.add_row({"database is empty"});
		std::cout << table_token << std::endl;
		exit(0);
	}

	table_token.add_row({"ID", "Note", "Username", "Expired"});
	for (int i = 0; i < result[0].size(); ++i)
	{
		table_token.add_row({result[0][i], result[1][i], result[2][i], result[3][i]});
	}

	table_token.column(3).format()
		.font_align(tabulate::FontAlign::right);
	for (size_t i = 0; i < 4; ++i)
	{
		table_token[0][i].format()
			.font_color(tabulate::Color::yellow)
			.font_align(tabulate::FontAlign::center)
			.font_style({tabulate::FontStyle::bold});
	}

	std::cout << table_token << std::endl;
}

void GAH::action_search_token(const int argc, const char *const argv[])
{
	argparse::ArgumentParser program = GAH::argparse_init();

	program.add_argument("filter")
		.default_value(std::string(""))
		.help("filter by [id | note | username | expired]")
		.action([](const std::string& value) {
			static const std::vector<std::string> filter_type{"id", "note", "username", "expired"};
			if (std::find(filter_type.begin(), filter_type.end(), value) != filter_type.end())
				return value;
			return std::string("");
		});
	program.add_argument("query")
		.default_value(std::string(""))
		.help("search query");

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		std::cout << err.what() << std::endl;
		std::cout << program;
		exit(1);
	}

	std::vector<std::string> query_keys{
		"filter",
		"query",
	};
	std::map<std::string, std::string> query;
	std::cout << "# search token" << std::endl;

	for (const std::string key: query_keys)
	{
		std::string str_arg = program.get(key);
		if (str_arg.empty())
		{
			std::cout << "insert " << key << ": ";
			std::getline(std::cin, str_arg);
		}
		else
		{
			std::cout << key << ": " << str_arg << std::endl;
		}
		query.insert({key, str_arg});
	}

	std::vector<std::vector<std::string>> result;
	GAH::db_get_token_list_by_filter(result, query["filter"], query["query"]);

	tabulate::Table table_token;

	if (!result[0].size())
	{
		table_token.add_row({"token not found"});
		std::cout << table_token << std::endl;
		exit(0);
	}

	table_token.add_row({"ID", "Note", "Username", "Expired"});
	for (int i = 0; i < result[0].size(); ++i)
	{
		table_token.add_row({result[0][i], result[1][i], result[2][i], result[3][i]});
	}

	table_token.column(3).format()
		.font_align(tabulate::FontAlign::right);
	for (size_t i = 0; i < 4; ++i)
	{
		table_token[0][i].format()
			.font_color(tabulate::Color::yellow)
			.font_align(tabulate::FontAlign::center)
			.font_style({tabulate::FontStyle::bold});
	}

	std::cout << table_token << std::endl;
}

void GAH::action_clipboard_token(const int argc, const char *const argv[])
{
	argparse::ArgumentParser program = GAH::argparse_init();

	program.add_argument("id")
		.default_value(std::string(""))
		.help("id token");

	program.add_argument("-p", "--password")
		.default_value(std::string(""))
		.help("input password directly from command");

	program.add_argument("-s", "--show")
		.default_value(false)
		.implicit_value(true)
		.help("show token");

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		std::cout << err.what() << std::endl;
		std::cout << program;
		exit(1);
	}

	std::vector<std::string> query_keys{
		"id",
	};
	std::map<std::string, std::string> query;
	std::cout << "# copy token to clipboard" << std::endl;

	for (const std::string key: query_keys)
	{
		std::string str_arg = program.get(key);
		if (str_arg.empty())
		{
			std::cout << "insert " << key << ": ";
			std::getline(std::cin, str_arg);
		}
		else
		{
			std::cout << key << ": " << str_arg << std::endl;
		}
		query.insert({key, str_arg});
	}

	std::string username;
	std::string token;
	GAH::db_get_username_and_token_from_id(query["id"], username, token);

  std::string password_arg = program.get<std::string>("--password");
	std::string password;
  if(password_arg.empty())
  {
	  GAH::utility_getline_password(password, username);
  }
  else
  {
    password = password_arg;
  }

	try
	{
		token = GAH::decrypt_token(token, password);
	}
	catch(const std::invalid_argument& e)
	{
		std::cout << e.what() << std::endl;
		exit(1);
	}

	if(program.get<bool>("--show"))
	{
	  std::cout << "token: " << token << "\n";
	}

  clip::set_text(token);

	std::cout << "token has been copied to clipboard" << std::endl;
}


void GAH::action_add_token(const int argc, const char *const argv[])
{
	argparse::ArgumentParser program = GAH::argparse_init();

	program.add_argument("note")
		.default_value(std::string(""))
		.help("github token note");
	program.add_argument("username")
		.default_value(std::string(""))
		.help("github username");
	program.add_argument("token")
		.default_value(std::string(""))
		.help("github token");
	program.add_argument("expired")
		.default_value(std::string(""))
		.help("github token expired date");

	program.add_argument("-p", "--password")
		.default_value(std::string(""))
		.help("input password directly from command");

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		std::cout << err.what() << std::endl;
		std::cout << program;
		exit(1);
	}

	std::vector<std::string> query_keys{
		"note",
		"username",
		"token",
		"expired",
	};
	std::map<std::string, std::string> query;
	std::cout << "# add token" << std::endl;

	for (const std::string key: query_keys)
	{
		std::string str_arg = program.get(key);
		if (str_arg.empty())
		{
			std::cout << "insert " << key << ": ";
			std::getline(std::cin, str_arg);
		}
		else
		{
			std::cout << key << ": " << str_arg << std::endl;
		}
		query.insert({key, str_arg});
	}

  std::string password_arg = program.get<std::string>("--password");
	std::string password;
  if(password_arg.empty())
  {
	  GAH::utility_getline_password(password, query["username"]);
  }
  else
  {
    password = password_arg;
  }

	query["token"] = "GAH_FLAG_" + query["token"];

	GAH::db_add_token(
		query["note"],
		query["username"],
		GAH::encrypt_token(query["token"], password),
		query["expired"]
	);

	std::cout << "add token successful" << std::endl;
}

void GAH::action_remove_token(const int argc, const char *const argv[])
{
	argparse::ArgumentParser program = GAH::argparse_init();

	program.add_argument("id")
		.default_value(std::string(""))
		.help("id token");

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		std::cout << err.what() << std::endl;
		std::cout << program;
		exit(1);
	}

	std::vector<std::string> query_keys{
		"id",
	};
	std::map<std::string, std::string> query;
	std::cout << "# remove token" << std::endl;

	for (const std::string key: query_keys)
	{
		std::string str_arg = program.get(key);
		if (str_arg.empty())
		{
			std::cout << "insert " << key << ": ";
			std::getline(std::cin, str_arg);
		}
		else
		{
			std::cout << key << ": " << str_arg << std::endl;
		}
		query.insert({key, str_arg});
	}

	GAH::db_statement = "DELETE FROM token WHERE id = ?";
	sqlite3_prepare(GAH::db, GAH::db_statement.c_str(), -1, &GAH::db_statement_step, NULL);
	try
	{
		sqlite3_bind_int(GAH::db_statement_step, 1, std::stoi(query["id"]));
	}
	catch(...)
	{
		std::cout << "id is not valid" << std::endl;
		exit(1);
	}
	sqlite3_step(GAH::db_statement_step);
	sqlite3_finalize(GAH::db_statement_step);
}

void GAH::action_git_push(const int argc, const char *const argv[])
{
	argparse::ArgumentParser program = GAH::argparse_init();

	program.add_argument("remote")
		.default_value(std::string(""))
		.help("git remote name");
	program.add_argument("branch")
		.default_value(std::string(""))
		.help("git branch name");
	program.add_argument("id")
		.default_value(std::string(""))
		.help("id token");

	program.add_argument("-p", "--password")
		.default_value(std::string(""))
		.help("input password directly from command");

	program.add_argument("-f", "--force")
		.default_value(false)
		.implicit_value(true)
		.help("force updates");

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		std::cout << err.what() << std::endl;
		std::cout << program;
		exit(1);
	}

	GAH::git_check_command_and_local_repo();

	std::map<std::string, std::string> remotes = GAH::git_get_remotes();
	std::vector<std::string> branches = GAH::git_get_branches();

	if (remotes.size() == 0)
	{
		std::cout << "git remote not found" << std::endl;
		std::cout << "make sure you add git remote to repository" << std::endl;
		exit(1);
	}
	else if (branches.size() == 0)
	{
		std::cout << "git branch not found" << std::endl;
		std::cout << "make sure you add git branch to repository" << std::endl;
		exit(1);
	}

	std::vector<std::string> query_keys{
		"remote",
		"branch",
		"id",
	};
	std::map<std::string, std::string> query;
	std::cout << "# git push" << std::endl;

	for (const std::string key: query_keys)
	{
		std::string str_arg = program.get(key);
		if (str_arg.empty())
		{
			std::cout << "insert " << key << ": ";
			std::getline(std::cin, str_arg);
		}
		else
		{
			std::cout << key << ": " << str_arg << std::endl;
		}
		query.insert({key, str_arg});
	}

	if(remotes.find(query["remote"]) == remotes.end())
	{
		std::cout << "remote " << query["remote"] <<" not available" << std::endl;
		exit(1);
	}
	else if(std::find(branches.begin(), branches.end(), query["branch"]) == branches.end())
	{
		std::cout << "branch " << query["branch"] <<" not available" << std::endl;
		exit(1);
	}

	std::string username;
	std::string token;
	GAH::db_get_username_and_token_from_id(query["id"], username, token);

  std::string password_arg = program.get<std::string>("--password");
	std::string password;
  if(password_arg.empty())
  {
	  GAH::utility_getline_password(password, username);
  }
  else
  {
    password = password_arg;
  }

	try
	{
		token = GAH::decrypt_token(token, password);
	}
	catch(const std::invalid_argument& e)
	{
		std::cout << e.what() << std::endl;
		exit(1);
	}

	std::string construct_command("git push " + GAH::github_construct_remote(username, token, remotes[query["remote"]]) + " " + query["branch"]);

	const std::vector<std::string> arg_key{
		"--force",
	};
	std::map<std::string, bool> key_with_boolean{};
	for (const std::string key: arg_key)
	{
		if(program.get<bool>(key))
			key_with_boolean.insert({key, true});
	}
	GAH::github_construct_command(construct_command, key_with_boolean);

	// const std::vector<std::string> arg_key_value{
	// 	"--args",
	// };
	// std::map<std::string, std::string> key_with_value{};
	// for (const std::string key: arg_key_value)
	// {
	// 	const std::string arg_value = program.get<std::string>(key);
	// 	if(arg_value != "")
	// 		key_with_value.insert({key, arg_value});
	// }
	// GAH::github_construct_command(construct_command, key_with_value);

	const char* command = construct_command.c_str();

	system(command);
}

void GAH::action_git_pull(const int argc, const char *const argv[])
{
	argparse::ArgumentParser program = GAH::argparse_init();

	program.add_argument("remote")
		.default_value(std::string(""))
		.help("git remote name");
	program.add_argument("branch")
		.default_value(std::string(""))
		.help("git branch name");
	program.add_argument("id")
		.default_value(std::string(""))
		.help("id token");

	program.add_argument("-p", "--password")
		.default_value(std::string(""))
		.help("input password directly from command");

	program.add_argument("-r", "--rebase")
		.default_value(false)
		.implicit_value(true)
		.help("incorporate changes by rebasing rather than merging");

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		std::cout << err.what() << std::endl;
		std::cout << program;
		exit(1);
	}

	GAH::git_check_command_and_local_repo();

	std::map<std::string, std::string> remotes = GAH::git_get_remotes();

	if (remotes.size() == 0)
	{
		std::cout << "git remote not found" << std::endl;
		std::cout << "make sure you add git remote to repository" << std::endl;
		exit(1);
	}

	std::vector<std::string> query_keys{
		"remote",
		"branch",
		"id",
	};
	std::map<std::string, std::string> query;
	std::cout << "# git pull" << std::endl;

	for (const std::string key: query_keys)
	{
		std::string str_arg = program.get(key);
		if (str_arg.empty())
		{
			std::cout << "insert " << key << ": ";
			std::getline(std::cin, str_arg);
		}
		else
		{
			std::cout << key << ": " << str_arg << std::endl;
		}
		query.insert({key, str_arg});
	}

	if(remotes.find(query["remote"]) == remotes.end())
	{
		std::cout << "remote " << query["remote"] <<" not available" << std::endl;
		exit(1);
	}

	std::string username;
	std::string token;
	GAH::db_get_username_and_token_from_id(query["id"], username, token);

  std::string password_arg = program.get<std::string>("--password");
	std::string password;
  if(password_arg.empty())
  {
	  GAH::utility_getline_password(password, username);
  }
  else
  {
    password = password_arg;
  }

	try
	{
		token = GAH::decrypt_token(token, password);
	}
	catch(const std::invalid_argument& e)
	{
		std::cout << e.what() << std::endl;
		exit(1);
	}

	std::string construct_command("git pull " + GAH::github_construct_remote(username, token, remotes[query["remote"]]) + " " + query["branch"]);

	const std::vector<std::string> arg_key{
		"--rebase",
	};
	std::map<std::string, bool> key_with_boolean{};
	for (const std::string key: arg_key)
	{
		if(program.get<bool>(key))
			key_with_boolean.insert({key, true});
	}
	GAH::github_construct_command(construct_command, key_with_boolean);

	// const std::vector<std::string> arg_key_value{
	// 	"--args",
	// };
	// std::map<std::string, std::string> key_with_value{};
	// for (const std::string key: arg_key_value)
	// {
	// 	const std::string arg_value = program.get<std::string>(key);
	// 	if(arg_value != "")
	// 		key_with_value.insert({key, arg_value});
	// }
	// GAH::github_construct_command(construct_command, key_with_value);

	const char* command = construct_command.c_str();

	system(command);
}

void GAH::action_git_clone(const int argc, const char *const argv[])
{
	argparse::ArgumentParser program = GAH::argparse_init();

	program.add_argument("url")
		.default_value(std::string(""))
		.help("github url repository");
	program.add_argument("id")
		.default_value(std::string(""))
		.help("id token");
	program.add_argument("directory")
		.default_value(std::string(""))
		.help("directory output");

	program.add_argument("-p", "--password")
		.default_value(std::string(""))
		.help("input password directly from command");

	program.add_argument("--recursive", "--recurse-submodules")
		.default_value(false)
		.implicit_value(true)
		.help("initialize submodules in the clone");
	program.add_argument("-b", "--branch")
		.default_value(std::string(""))
		.help("checkout <branch> instead of the remote's HEAD");

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		std::cout << err.what() << std::endl;
		std::cout << program;
		exit(1);
	}

	if (!GAH::git_check_command())
	{
		std::cout << "git command not found" << std::endl;
		exit(1);
	}

	std::vector<std::string> query_keys{
		"url",
		"id",
	};
	std::map<std::string, std::string> query;
	std::cout << "# git clone" << std::endl;

	for (const std::string key: query_keys)
	{
		std::string str_arg = program.get(key);
		if (str_arg.empty())
		{
			std::cout << "insert " << key << ": ";
			std::getline(std::cin, str_arg);
		}
		else
		{
			std::cout << key << ": " << str_arg << std::endl;
		}
		query.insert({key, str_arg});
	}

	std::string username;
	std::string token;
	GAH::db_get_username_and_token_from_id(query["id"], username, token);

  std::string password_arg = program.get<std::string>("--password");
	std::string password;
  if(password_arg.empty())
  {
	  GAH::utility_getline_password(password, username);
  }
  else
  {
    password = password_arg;
  }

	try
	{
		token = GAH::decrypt_token(token, password);
	}
	catch(const std::invalid_argument& e)
	{
		std::cout << e.what() << std::endl;
		exit(1);
	}

	std::string construct_command("git clone " + GAH::github_construct_remote(username, token, query["url"]));
	const std::string directory = program.get("directory");
	if (!directory.empty())
	{
		construct_command += " \"" + directory + "\"";
	}

	const std::vector<std::string> arg_key{
		"--recursive",
	};
	std::map<std::string, bool> key_with_boolean{};
	for (const std::string key: arg_key)
	{
		if(program.get<bool>(key))
			key_with_boolean.insert({key, true});
	}
	GAH::github_construct_command(construct_command, key_with_boolean);

	const std::vector<std::string> arg_key_value{
		"--branch",
	};
	std::map<std::string, std::string> key_with_value{};
	for (const std::string key: arg_key_value)
	{
		const std::string arg_value = program.get<std::string>(key);
		if(arg_value != "")
			key_with_value.insert({key, arg_value});
	}
	GAH::github_construct_command(construct_command, key_with_value);

	construct_command += " && cd \"" + (!directory.empty() ? directory : GAH::github_get_dir_name_from_url(query["url"])) + "\"";
	construct_command += " && git remote set-url origin " + query["url"];
	const char* command = construct_command.c_str();

	system(command);
}

void GAH::main_actions(const int argc, const char *const argv[])
{
	argparse::ArgumentParser program("gah", std::string(GAH_VERSION));

	program.add_argument("action")
		.required()
		.remaining()
		.help("[list | search | clipboard | add | remove | push | pull | clone]");

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		std::cout << err.what() << std::endl;
		std::cout << program;
		exit(1);
	}

	GAH::EActions action;

	try
	{
		action = GAH::str_to_actions(program.get<std::string>("action"));
	}
	catch(...)
	{
		std::cout << program;
		exit(1);
	}

	if (action == GAH::NONE)
	{
		std::cout << program;
		exit(1);
	}

	switch(action)
	{
		case GAH::LIST:
			GAH::action_list_token(argc, argv);
			break;
		case GAH::SEARCH:
			GAH::action_search_token(argc, argv);
			break;
		case GAH::CLIPBOARD:
			GAH::action_clipboard_token(argc, argv);
			break;
		case GAH::ADD:
			GAH::action_add_token(argc, argv);
			break;
		case GAH::REMOVE:
			GAH::action_remove_token(argc, argv);
			break;
		case GAH::PUSH:
			GAH::action_git_push(argc, argv);
			break;
		case GAH::PULL:
			GAH::action_git_pull(argc, argv);
			break;
		case GAH::CLONE:
			GAH::action_git_clone(argc, argv);
			break;		
		default:
			break;
	}
}
