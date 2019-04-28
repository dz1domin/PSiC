#include "extra.h"

std::string getSuffix(std::string& str)
{
	int i = 0;
	unsigned s = str.size();
	try
	{
		while(str.at(s - i - 1) != '.')
			i++;
		return str.substr(s - i, s);
	}
	catch(...)
	{
		return "";
	}
}

string parseData(std::string& data)
{
	std::istringstream is(data);
	std::string line, word, method, path, version;
	std::getline(is, line);
	std::istringstream is_line(line);
	std::getline(is_line, method, ' ');
	std::getline(is_line, path, ' ');
	std::getline(is_line, version, ' ');

	std::string test = data.substr(data.find("\r\n"), data.find("\r\n\r\n") - data.find("\r\n"));

	if(method == "POST")
	{
		std::string record, key, value;
		std::string fieldStorage = data.substr(data.find("\r\n\r\n") + 4, data.size());
		
		std::istringstream is_amp(fieldStorage);
		while(std::getline(is_amp, record, '&'))
		{
			key = record.substr(0, record.find("="));
			value = record.substr(record.find("=") + 1, record.size());
			std::cout << std::endl << key << ":" << value;
		}
		std::cout << std::endl;
		if(!key.size() && !value.size())
			std::cout << fieldStorage;

		std::vector<char> res;
		std::string headers;
		std::string file_to_read;
		if(path == "/")
			file_to_read = rootdir + "/index.html";
		else
			file_to_read = rootdir + path;

		std::string suffix = getSuffix(file_to_read);

		try
		{
			res = readFile(file_to_read.data());	
		}
		catch(...)
		{
			headers = failure;
			std::cerr << "exception trying to load: " << file_to_read << std::endl;
			perror("readFile() error");
			return headers;
		}

		headers = success;
		headers += "Content-type:";

		if(suffix == "html")
			headers += "text/html;charset=utf-8\r\n";
		else if(suffix == "js")
			headers += "text/javascript\r\n";
		else if(suffix == "jpg")
			headers += "image/jpeg\r\n";
		else
			headers += "application/octet-stream\r\n";

		headers += "Content-length:";
		headers += std::to_string(res.size()) + "\r\n\r\n";
		string v(headers.begin(), headers.end());
		v.insert(v.end(), res.begin(), res.end());
		return v;
	}
	else if(method == "GET")
	{
		std::vector<char> res;
		std::string headers;
		std::string file_to_read;
		if(path == "/")
			file_to_read = rootdir + "/index.html";
		else
			file_to_read = rootdir + path;

		std::string suffix = getSuffix(file_to_read);

		try
		{
			res = readFile(file_to_read.data());	
		}
		catch(...)
		{
			headers = failure;
			std::cerr << "exception trying to load: " << file_to_read << std::endl;
			perror("readFile() error");
			return headers;
		}

		headers = success;
		headers += "Content-type:";

		if(suffix == "html")
			headers += "text/html;charset=utf-8\r\n";
		else if(suffix == "js")
			headers += "text/javascript\r\n";
		else if(suffix == "jpg")
			headers += "image/jpeg\r\n";
		else
			headers += "text\r\n";

		headers += "Content-length:";
		headers += std::to_string(res.size()) + "\r\n\r\n";
		string v(headers.begin(), headers.end());
		v.insert(v.end(), res.begin(), res.end());
		return v;
	}
	std::string s = failure;
	return s;
}

vector<char> readFile (const char* path)
{
  using namespace std;

  vector<char> result;
  ifstream file_to_read(path, ios::in|ios::binary|ios::ate);
  file_to_read.exceptions( ifstream::failbit | ifstream::badbit );
  result.resize(file_to_read.tellg(), 0);
  file_to_read.seekg(0, ios::beg);
  file_to_read.read(result.data(), result.size());
  file_to_read.close();

  return result;
}