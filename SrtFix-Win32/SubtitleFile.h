#pragma once
class SubtitleFile
{
private:
	std::wstring file_fullpath;
	std::wstring text;
	enum Encoding encoding;
public:
	enum Encoding { UTF8, UTF16, ANSI };
	SubtitleFile(std::wstring file_fullpath);
	~SubtitleFile();
	Encoding GetEncoding();
	void ApplyProperLineSeparation();
	bool IsValid();//To check that the filepath that you gave it is a valid file
};

