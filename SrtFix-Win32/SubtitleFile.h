#pragma once

enum Encoding { UTF8, UTF16, ANSI };

class SubtitleFile
{
private:
	std::wstring file_fullpath;
	std::wstring text;
	Encoding encoding;
public:
	SubtitleFile(std::wstring file_fullpath);//TODO(fran): we could throw an exception at creation time if file is invalid
	~SubtitleFile();

	Encoding		GetEncoding();
	void			SetEncoding(Encoding encoding);
	void			SetEncodingFromFile();

	std::wstring	GetFile_FullPath();
	std::wstring	GetFile_Name();
	std::wstring	GetFile_Extension();
	std::wstring	GetFile_Directory();
	void			SetFile(std::wstring file_fullpath);
	
	std::wstring	GetText();
	void			SetText(std::wstring text);
	
	void			ApplyProperLineSeparationToText();//TODO(fran): private?
	void			ReadFileToText();
};

