//#include "stdafx.h"
#include "SubtitleFile.h"


SubtitleFile::SubtitleFile()
{
}

SubtitleFile::SubtitleFile(std::wstring file_fullpath)
{
	this->file_fullpath = file_fullpath;
	//TODO(fran): error checking
}


SubtitleFile::~SubtitleFile()
{
	//TODO(fran): do I need to do this?
	this->file_fullpath.~basic_string();
	this->text.~basic_string();
}

///analize for ascii,utf8,utf16
void SubtitleFile::SetEncodingFromFile() {

	char header[4]; //no usar wchar asi leo de a un byte, lo mismo con ifstream en vez de wifstream
	std::ifstream file(file_fullpath, std::ios::in | std::ios::binary); //@@Can this read unicode filenames??
	if (!file) {
		this->encoding = Encoding::UTF8;//parcheo rapido de momento
		return;
	}
	else {
		file.read(header, 3);
		file.close();

		if (header[0] == (char)0XEF && header[1] == (char)0XBB && header[2] == (char)0XBF) {
			this->encoding = Encoding::UTF8;
			return;
		}
		else if (header[0] == (char)0XEF && header[1] == (char)0XBB) {
			this->encoding = Encoding::UTF8;
			return;
		}
		else if (header[0] == (char)0xFF && header[1] == (char)0xFE) {
			this->encoding = Encoding::UTF16;//little-endian
			return;
		}
		else if (header[0] == (char)0xFE && header[1] == (char)0xFF) {
			this->encoding = Encoding::UTF16;//big-ending
			return;
		}
		else {//do manual checking
			std::wifstream file(file_fullpath, std::ios::in | std::ios::binary);//@@no se si wifstream o ifstream (afecta a stringstream)
			std::wstringstream buffer;
			buffer << file.rdbuf();
			file.close();
			buffer.seekg(0, std::ios::end);
			int length = buffer.tellg();//long enough length
			buffer.clear();
			buffer.seekg(0, std::ios::beg);
			//https://unicodebook.readthedocs.io/guess_encoding.html
			//NOTE: creo q esto testea UTF16 asi que probablemente haya errores, y toma ascii como utf
			int test;
			test = IS_TEXT_UNICODE_ILLEGAL_CHARS;
			if (IsTextUnicode(buffer.str().c_str(), length, &test)) {
				this->encoding = Encoding::ANSI;
				return;
			} 
			test = IS_TEXT_UNICODE_STATISTICS;
			if (IsTextUnicode(buffer.str().c_str(), length, &test)) {
				this->encoding = Encoding::UTF8;
				return;
			}
			test = IS_TEXT_UNICODE_REVERSE_STATISTICS;
			if (IsTextUnicode(buffer.str().c_str(), length, &test)) {
				this->encoding = Encoding::UTF8;
				return;
			}
			test = IS_TEXT_UNICODE_ASCII16;
			if (IsTextUnicode(buffer.str().c_str(), length, &test)) {
				this->encoding = Encoding::UTF8;
				return;
			}
			this->encoding = Encoding::ANSI;
			return;
		}
	}
}

Encoding SubtitleFile::GetEncoding()
{
	return this->encoding;
}

///In case the chosen one was incorrect
void SubtitleFile::SetEncoding(Encoding encoding)
{
	this->encoding = encoding;
}

///Every line separation(\r or \n or \r\n) will be transformed into \r\n
void SubtitleFile::ApplyProperLineSeparationToText() {
	unsigned int pos = 0;

	while (pos <= this->text.length()) {//length doesnt include \0
		if (this->text[pos] == L'\r') {
			if (this->text[pos + 1] != L'\n') {
				this->text.insert(pos + 1, L"\n");//este nunca se pasa del rango
			}
			pos += 2;
		}
		else if (this->text[pos] == L'\n') {
			//if(text[pos-1] == L'\r')//ya se que no tiene \r xq sino hubiera entrado por el otro
			this->text.insert(pos, L"\r");
			pos += 2;
		}
		else {
			pos++;
		}
	}
}

void SubtitleFile::ReadFileToText() {
	//TODO(fran): Should I do getencoding first, or assume it was already requested and is updated
	//TODO(fran): Should this be done in another thread?

	std::wifstream file(this->file_fullpath, std::ios::binary);

	if (file.is_open()) {

		//set encoding for getline
		if (this->encoding == UTF8)
			file.imbue(std::locale::locale(file.getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::consume_header>));
		else if (this->encoding == UTF16)
			file.imbue(std::locale::locale(file.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));
		//if encoding is ASCII we do nothing

		std::wstringstream buffer;
		buffer << file.rdbuf();
		//wstring for_testing = buffer.str();
		this->text = buffer.str();

		file.close();
	}
}

void SubtitleFile::SetFile(std::wstring file_fullpath)
{
	//TODO(fran): I think when the path is changed the other variables should be cleared to zero
	//            also we should check whether the file is valid, and possibly read it and everything?
	this->file_fullpath = file_fullpath;
}

std::wstring SubtitleFile::GetFile_FullPath()
{
	//TODO(fran): error checking
	return this->file_fullpath;
}

std::wstring SubtitleFile::GetFile_Name()
{
	//TODO(fran): check there is no by 1 error
	//TODO(fran): error checking
	size_t first_letter_pos = this->file_fullpath.find_last_of(L"\\") + 1;
	return this->file_fullpath.substr(first_letter_pos, this->file_fullpath.find_last_of(L".") - first_letter_pos);
}

std::wstring SubtitleFile::GetFile_Extension()
{
	//TODO(fran): error checking
	return this->file_fullpath.substr(this->file_fullpath.find_last_of(L".") + 1);
}

std::wstring SubtitleFile::GetFile_Directory()
{
	//TODO(fran): esto incluye o no la ultima barra?
	//TODO(fran): error checking
	return this->file_fullpath.substr(0, this->file_fullpath.find_last_of(L"\\") + 1);
}

std::wstring SubtitleFile::GetText()
{
	return this->text;
}

void SubtitleFile::SetText(std::wstring text)
{
	this->text = text;
}
