

//this might be used to create a library for debugging, so:
#ifndef DEBUG
#define DEBUG
#endif
#include "output.hpp"

#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <ctime>
#include <mutex>
#include <atomic>
#include <fstream>

namespace dlo{

using std::to_string;
using std::cout;
using std::cerr;
using std::endl;


string default_get_note_prefix(int level);

//in fact, those are like private members:
namespace impl{
	static std::atomic_int verbose_level{0};
	static std::atomic_int debug_level{0};
	static std::unique_ptr<std::ostream> logfile;
	static std::mutex write_mutex;
	static std::mutex note_prefix_mutex;
	
	static std::function<void(const std::string&)> write_normal
		= [](const std::string& text){cout << text << endl;};
	static std::function<void(const std::string&)> write_error
		= [](const std::string& text){cerr << text << endl;};
	static std::function<string(int level)> get_note_prefix = default_get_note_prefix;
}

void set_verbosity(int level){
	impl::verbose_level=level;
}

int set_logfile(const string& filename){
	std::unique_ptr<std::ostream> new_file {};
	if(!filename.empty()){
		new_file = std::unique_ptr<std::ostream>{new std::ofstream{filename}};
		if(!static_cast<std::ofstream*>(new_file.get())->is_open()){
			return 1;
		}
	}
	std::lock_guard<std::mutex> guard{impl::write_mutex};
	std::swap(new_file, impl::logfile);
	return 0;
}

int set_logfile(std::unique_ptr<std::ostream>&& new_output){
	std::lock_guard<std::mutex> guard{impl::write_mutex};
	std::swap(new_output, impl::logfile);
	return 0;
}

void _debug(const string& filename, const string& function_name, int line, int level, const string& text){
	if(level<=impl::debug_level){
		string metadata = stringutils::textf("DEBUG(%s) [“%s”, %s (#%s)]: ", 
				level, filename, function_name, line);
		print_and_log(metadata, text);
	}
}

void set_debug_level(int d){
	impl::debug_level=d;
}

void _writeln(const string& text){
	print_and_log(text);
}

void _writeln(std::ostream& stream, const string& text){
	stream << text << endl;
}

void _note(int level, const string& text){
	if(level <= impl::verbose_level ){
		std::string prefix;
		{ // synchronize
			std::lock_guard<std::mutex> guard{impl::note_prefix_mutex};
			prefix = impl::get_note_prefix(level);
		}
		print_and_log(prefix, text);
	}
}

void _warn(const string& text){
	print_and_log("WARNING: ", text, false);
}

void _error(const string& text){
	print_and_log("ERROR: ", text, false);
}

namespace {
class fatal_error_exception{
};
}

void _fatal(const string& text){
	print_and_log("FATAL: ", text, false);
	throw fatal_error_exception();
}

string get_timestamp(){
	std::vector<char> buffer(30,0);
	time_t t = time(NULL);
	
	strftime(buffer.data(), buffer.size()-1, "%x %X", localtime(&t));
	
	string returnstr = buffer.data();
	return returnstr;
}

void print_and_log(const string& prefix, const string& msg, bool normal){
	print_and_log( stringutils::prefix_and_align(prefix, msg), normal);
}

void print_and_log(const string& msg, bool normal){
	if( normal ){
		std::lock_guard<std::mutex> guard{impl::write_mutex};
		impl::write_normal( msg );
	}
	else{
		std::lock_guard<std::mutex> guard{impl::write_mutex};
		impl::write_error( msg );
	}
	string str = stringutils::prefix_and_align( "[" + get_timestamp() + "] ", msg + '\n');
	std::lock_guard<std::mutex> guard{impl::write_mutex};
	if(impl::logfile){
		if(!((*impl::logfile) << str)){
			throw std::runtime_error{"could not write to logfile"};
		}
	}
}

void set_stdout_fun(std::function<void(const string&)> fun){
	std::lock_guard<std::mutex> guard{impl::write_mutex};
	impl::write_normal = fun;
}

void set_stderr_fun(std::function<void(const string&)> fun){
	std::lock_guard<std::mutex> guard{impl::write_mutex};
	impl::write_error = fun;
}

void set_note_prefix_fun(std::function<string(int)> fun){
	if(fun == nullptr){
		std::lock_guard<std::mutex> guard{impl::note_prefix_mutex};
		impl::get_note_prefix = default_get_note_prefix;
	}
	else{
		std::lock_guard<std::mutex> guard{impl::note_prefix_mutex};
		impl::get_note_prefix = fun;
	}
}

string default_get_note_prefix(int level){
	if(level == 0){
		return "";
	}
	else{
		return "NOTE(" + to_string(level) + "): ";
	}
}


} //namespace logging
