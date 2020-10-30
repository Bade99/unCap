#pragma once
#include <string>
#include "unCap_Platform.h"
#include <sstream>
#include <locale>
#include <codecvt>
#include <fstream>
#include <intrin.h>

enum TXT_ENCODING {
	ANSI = 1, //TODO(fran): ansi may be too generic
	UTF8,
	UTF16, //Big endian or Little endian
	//ASCII is easily encoded in utf8 without problems
};

TXT_ENCODING GetTextEncoding(std::wstring filename) {

	HANDLE file;
	DWORD  dwBytesRead = 0;
	BYTE   header[4] = { 0 };

	file = CreateFile(filename.c_str(),// file to open
		GENERIC_READ,          // open for reading
		FILE_SHARE_READ,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL, // normal file
		NULL);                 // no attr. template

	Assert(file != INVALID_HANDLE_VALUE);

	BOOL read_res = ReadFile(file, header, 4, &dwBytesRead, NULL);
	Assert(read_res);

	CloseHandle(file);

	if (header[0] == 0xEF && header[1] == 0xBB && header[2] == 0xBF)	return TXT_ENCODING::UTF8; //File has UTF8 BOM
	else if (header[0] == 0xFF && header[1] == 0xFE)		return TXT_ENCODING::UTF16; //File has UTF16LE BOM, INFO: this could actually be UTF32LE but I've never seen a subtitle file enconded in utf32 so no real need to do the extra checks
	else if (header[0] == 0xFE && header[1] == 0xFF)		return TXT_ENCODING::UTF16; //File has UTF16BE BOM
	else {//Do manual check

		//Check for UTF8

		std::ifstream ifs(filename); //TODO(fran): can ifstream open wstring (UTF16 encoded) filenames?
		Assert(ifs);

		std::istreambuf_iterator<char> it(ifs.rdbuf());
		std::istreambuf_iterator<char> eos;

		bool isUTF8 = utf8::is_valid(it, eos); //there's also https://unicodebook.readthedocs.io/guess_encoding.html

		ifs.close();

		if (isUTF8) return TXT_ENCODING::UTF8; //File is valid UTF8

		//Check for UTF16

		file = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); //Open file for reading
		Assert(file != INVALID_HANDLE_VALUE);

		BYTE fixedBuffer[50000] = { 0 }; //We dont need the entire file, just enough data for the functions to be as accurate as possible. Pd. this is probably too much already
		read_res = ReadFile(file, fixedBuffer, 50000, &dwBytesRead, NULL);
		Assert(read_res);

		CloseHandle(file);

		AutoIt::Common::TextEncodingDetect textDetect;
		bool isUTF16 = textDetect.isUTF16(fixedBuffer, dwBytesRead);

		if (isUTF16) return TXT_ENCODING::UTF16;

		//Give up and return ANSI
		return TXT_ENCODING::ANSI;
	}
}

struct GetTextEncodingResult { TXT_ENCODING encoding; bool is_big_endian; bool has_bom; };
//NOTE: intel is little endian
GetTextEncodingResult GetTextEncoding(const u8* buf, u32 sz) {
	GetTextEncodingResult res;
	using AutoIt::Common::TextEncodingDetect;
	TextEncodingDetect txtinspect; 
	TextEncodingDetect::Encoding enc = txtinspect.DetectEncoding(buf,sz);
	switch (enc) {
	case TextEncodingDetect::Encoding::None:
	case TextEncodingDetect::Encoding::ANSI:
	case TextEncodingDetect::Encoding::ASCII:
	case TextEncodingDetect::Encoding::UTF8_BOM:
	case TextEncodingDetect::Encoding::UTF8_NOBOM:
	{
		res.encoding = TXT_ENCODING::UTF8;
	} break;
	case TextEncodingDetect::Encoding::UTF16_LE_BOM:
	case TextEncodingDetect::Encoding::UTF16_LE_NOBOM:
	case TextEncodingDetect::Encoding::UTF16_BE_BOM:
	case TextEncodingDetect::Encoding::UTF16_BE_NOBOM:
	{
		res.encoding = TXT_ENCODING::UTF16;
	} break;
	default: 
	{
		Assert(0);
	} break;
	}//NOTE: we wont be needing ansi, we simply use utf8
	switch (enc) {
	case TextEncodingDetect::Encoding::UTF8_BOM:
	case TextEncodingDetect::Encoding::UTF16_LE_BOM:
	case TextEncodingDetect::Encoding::UTF16_BE_BOM:
	{
		res.has_bom = true;
	} break;
	default:
	{
		res.has_bom = false;
	} break;
	}
	switch (enc) {
	case TextEncodingDetect::Encoding::UTF16_BE_BOM:
	case TextEncodingDetect::Encoding::UTF16_BE_NOBOM:
	{
		res.is_big_endian = true;
	} break;
	default:
	{
		res.is_big_endian = false;
	} break;
	}
	return res;
}

struct ReadTextResult { std::wstring text; TXT_ENCODING encoding; };
#if 0
ReadTextResult ReadText(const cstr* filepath) {
	//NOTE: on debug mode it takes 480ms!! to execute this whole function to process the 108KB subtitle for Amadeus, simply pathetic
	//on release it goes down to 10ms, still no good
	i64 cnt = StartCounter(); defer{ printf("READ ELAPSED: %f ms\n",EndCounter(cnt)); };

	ReadTextResult res;

	res.encoding = GetTextEncoding(filepath);//TODO(fran): this begins stupid, I need the bytes in ram first, then I can analyze all I want, right now Im doing two reads from disk of the same file
	printf("Encoding Read: %d\n", res.encoding);

	std::wifstream file(filepath, std::ios::binary);

	if (file.is_open()) {

		//set encoding for getline
		if (res.encoding == TXT_ENCODING::UTF8)
			file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::consume_header>));//TODO(fran): shouldnt I use codecvt_utf8_utf16?
		//maybe the only thing needed when utf8 is detected is to remove the BOM if it is there
		else if (res.encoding == TXT_ENCODING::UTF16)
			file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));
		//if encoding is ANSI we do nothing <-TODO(fran): is this correct?

		std::wstringstream buffer;
		buffer << file.rdbuf(); //TODO(fran): this is horribly slow, just useless
		res.text = buffer.str();

		file.close();
	}
	return res;
}
#else
//NOTE: thanks to being on windows we normalize to utf16/ucs2
//TODO(fran): we're very close to not needing string, we still have FixLineEndings
ReadTextResult ReadText(const cstr* filepath) {
	//TODO(fran): look at https://docs.microsoft.com/en-us/windows/win32/intl/using-unicode-normalization-to-represent-strings
	//NOTE: on debug mode it takes 1ms!!!! to execute this whole function to process the 108KB subtitle for Amadeus, simply amazing compared to the other garbage, 480 times faster and we are running MultiByteToWideChar twice, we could preallocate say twice the original sz which would cover us 99% of the times
	//on release it goes a little faster 0.9ms - 0.7ms
	i64 cnt = StartCounter(); defer{ printf("READ ELAPSED: %f ms\n",EndCounter(cnt)); };

	ReadTextResult res;

	auto read_res = read_entire_file(filepath);

	if (read_res.mem) {

		GetTextEncodingResult enc_res = GetTextEncoding((u8*)read_res.mem, read_res.sz);
		res.encoding = enc_res.encoding;
		printf("Encoding Read: %d\n", res.encoding);

		//TODO(fran): consume BOM if present
		//TODO(fran): big vs little endian utf16

		if (res.encoding == TXT_ENCODING::UTF8) {
			//INFO: MultiByteToWideChar already takes care of removing the BOM
			int sz_char = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)read_res.mem, read_res.sz, 0, 0);//NOTE: pre windows vista this may need a change
				if (sz_char) {
					void* mem = VirtualAlloc(0, sz_char * sizeof(WCHAR), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
					if (mem) {
						MultiByteToWideChar(CP_UTF8, 0, (LPCCH)read_res.mem, read_res.sz, (LPWSTR)mem, sz_char);
						res.text = (WCHAR*)mem;
						VirtualFree(mem, 0, MEM_RELEASE);
					}
				}
		}
		else if (res.encoding == TXT_ENCODING::UTF16) {
			if (enc_res.is_big_endian) {//gotta convert to little endian on windows
				for (u32 i = 0, sz_char= read_res.sz/sizeof(WCHAR); i < sz_char; i++) {//TODO(fran):faster!!
					((WCHAR*)read_res.mem)[i] = _byteswap_ushort(((WCHAR*)read_res.mem)[i]);
				}
			}
			res.text = (WCHAR*)read_res.mem;//INFO: strangely enough std::wstring also takes care of removing the BOM, even though they dont handle loading big endian format
		}
		else {
			res.text = (WCHAR*)read_res.mem;
		}
		free_file_memory(read_res.mem);
	}
	return res;
}
#endif

enum FILE_FORMAT {
	SRT = 1, //This can also be used for any format that doesnt use in its sintax any of the characters detected as "begin comment" eg [ { (
	SSA //Used to signal any version of Substation Alpha: v1,v2,v3,v4,v4+ (Advanced Substation Alpha)
};

FILE_FORMAT GetFileFormat(std::wstring filename) {
	size_t extesion_pos = 0;
	extesion_pos = filename.find_last_of(L'.', std::wstring::npos); //Look for file extesion
	if (extesion_pos != std::wstring::npos && (extesion_pos + 1) < filename.length()) {
		//We found and extension and checked that there's at least one char after the ".", lets see if we support it
		//TODO(fran): create some sort of a map FILE_FORMAT-wstring eg FILE_FORMAT::SRT - L"srt" and a way to enumerate every FILE_FORMAT

		//TODO(fran): make every character in the filename that we're going to compare lowercase
		WCHAR srt[] = L"srt";
		if (!filename.compare(extesion_pos + 1, ARRAYSIZE(srt) - 1, srt) && (filename.length() - (extesion_pos + 1)) == (ARRAYSIZE(srt) - 1))
			return FILE_FORMAT::SRT;
		//TODO(fran): check the -1 is correct
		WCHAR ssa[] = L"ssa";
		if (!filename.compare(extesion_pos + 1, ARRAYSIZE(ssa) - 1, ssa) && (filename.length() - (extesion_pos + 1)) == (ARRAYSIZE(ssa) - 1))
			return FILE_FORMAT::SSA;

		WCHAR ass[] = L"ass";
		if (!filename.compare(extesion_pos + 1, ARRAYSIZE(ass) - 1, ass) && (filename.length() - (extesion_pos + 1)) == (ARRAYSIZE(ass) - 1))
			return FILE_FORMAT::SSA;
	}
	else {
		//File has no extension, do manual checking. Note that we are either checking the extesion or using manual checks but not both,
		//the idea behind it is that if the file has an extension and we couldn't find it between our supported list then we dont support it,
		//since, for now, we trust that what the extension says is true

		//TODO(fran): manual checking, aka reading the file

	}

	//If everything else fails fallback to srt //TODO(fran): we could also reject the file, adding an INVALID into FILE_FORMAT
	return FILE_FORMAT::SRT;
}

//The different types of characters that are detected as comments
enum COMMENT_TYPE {
	brackets = 0, parenthesis, braces, other
};

COMMENT_TYPE GetCommentType(const std::wstring &text) {
	if (text.find_first_of(L'{') != std::wstring::npos) {
		return COMMENT_TYPE::braces;
	}
	else if (text.find_first_of(L'[') != std::wstring::npos) {
		return COMMENT_TYPE::brackets;
	}
	else if (text.find_first_of(L'(') != std::wstring::npos) {
		return COMMENT_TYPE::parenthesis;
	}
	else {
		return COMMENT_TYPE::other;
	}
}

/// <summary>
/// Performs comment removal for .srt type formats
/// </summary>
/// <returns>The number of comments removed, 0 indicates no comments were found</returns>
size_t CommentRemovalSRT(std::wstring& text, utf16 start, utf16 end) {
	size_t comment_count = 0;
	size_t start_pos = 0, end_pos = 0;

	while (true) {
		start_pos = text.find(start, start_pos);
		end_pos = text.find(end, start_pos + 1);//+1, dont add it here 'cause breaks the if when end_pos=-1(string::npos)
		if (start_pos == std::wstring::npos || end_pos == std::wstring::npos) {
			//SendMessage(hRemoveProgress, PBM_SETPOS, text_length, 0);
			break;
		}
		text.erase(text.begin() + start_pos, text.begin() + end_pos + 1); //TODO(fran): handle exceptions
		comment_count++;
		//SendMessage(hRemoveProgress, PBM_SETPOS, start_pos, 0);
	}
	return comment_count;
}

/// <summary>
/// Performs comment removal for all .ssa versions including .ass (v1,v2,v3,v4,v4+)
/// </summary>
/// <returns>The number of comments removed, 0 indicates no comments were found</returns>
size_t CommentRemovalSSA(std::wstring& text, utf16 start, utf16 end) {

	// Removing comments for Substation Alpha format (.ssa) and Advanced Substation Alpha format (.ass)
	//https://wiki.multimedia.cx/index.php/SubStation_Alpha
	//https://matroska.org/technical/specs/subtitles/ssa.html
	//INFO: ssa has v1, v2, v3 and v4, v4+ is ass but all of them are very similar in what concerns us
	//I only care about lines that start with Dialogue:
	//This lines are always one liners, so we could use getline or variants
	//This lines can contain extra commands in the format {\command} therefore in case we are removing braces "{" we must check whether the next char is \ and ignore in that case
	//The same rule should apply when detecting the type of comments in CommentTypeFound
	//If we find one of this extra commands inside of a comment we remove it, eg [Hello {\br} goodbye] -> Remove everything


	//TODO(fran): check that a line with no text is still valid, eg if the comment was the only text on that Dialogue line after we remove it there will be nothing

	size_t comment_count = 0;
	size_t start_pos = 0, end_pos = 0;
	size_t line_pos = 0;
	utf16 text_marker[10] = L"Dialogue:";
	size_t text_marker_size = 10;

	//Substation Alpha has a special code for identifying text that will be shown to the user: 
	//The line has to start with "Dialogue:"
	//Also this lines can contain additional commands in the form {\command} 
	//therefore we've got to be careful when "start" is "{" and also check the next char for "\"
	//in which case it's not actually a comment and we should just ignore it and continue

	while (true) {
		//Find the "start" character
		start_pos = text.find(start, start_pos);
		if (start_pos == std::wstring::npos) break;

		//Check that the character is in a line that start with "Dialogue:"
		line_pos = text.find_last_of(L'\n', start_pos);
		if (line_pos == std::wstring::npos) line_pos = 0; //We are on the first line
		else line_pos++; //We need to start checking from the next char after the end of line, eg "\nDialogue:" we want the "D"
		if (text.compare(line_pos, text_marker_size - 1, text_marker)) {//if true the line where we found a possible comment does not start with the desired string
			start_pos++; //Since we made no changes to the string at start_pos we have to move start_pos one char forward so we dont get stuck 
			continue;
		}

		//Now lets check in case "start" is "{" whether the next char is "\"
		if (start == L'{') { //TODO(fran): can this be embedded in the code below?
			if ((start_pos + 1) >= text.length()) break; //Check a next character exists after our current position
			if (text.at(start_pos + 1) == L'\\') { //Indeed it was a \ so we where inside a special command, just ignore it and continue searching
				start_pos++; //Since we made no changes to the string at start_pos we have to move start_pos one char forward so we dont get stuck 
				continue;
			}
		}

		//TODO(fran): there's a bigger check that we need to perform, some of the commands inside {\command} can have parenthesis()
		// So what we should do is always check if we are inside of a command in which case we ignore and continue
		// How to perform the check: we have to go back from our position and search for the closest "{\" in our same line, but also check that there is
		// no "}" that is closer
		size_t command_begin_pos = text.rfind(L"{\\", start_pos); //We found a command, are we inside it?
		if (command_begin_pos != std::wstring::npos) {
			if (command_begin_pos >= line_pos) {//Check that the "{\" is on the same line as us
				//INFO: we are going to assume that nesting {} or {\} inside a {\} is invalid and is never used, otherwise add this extra check (probably needs a loop)
				//size_t possible_comment_begin_pos; //Closest "{" going backwards from our current position
				//possible_comment_begin_pos = text.find_last_of(L'{', start_pos);
				size_t possible_comment_end_pos; //Closest "}" going forwards from command begin position
				possible_comment_end_pos = text.find_first_of(L'}', command_begin_pos);
				if (possible_comment_end_pos == std::wstring::npos) break; //There's a command that has no end, invalid
				//TODO(fran): add check if the comment end is on the same line
				if (possible_comment_end_pos > start_pos) { //TODO(fran): what to do if they are equal??
					start_pos++;
					continue; //We are inside of a command, ignore and continue
				}
			}
		}

		end_pos = text.find(end, start_pos + 1); //Search for the end of the comment
		if (end_pos == std::wstring::npos) break; //There's no "comment end" so lets just exit the loop, the file is probably incorrectly written

		text.erase(text.begin() + start_pos, text.begin() + end_pos + 1); //Erase the entire comment, including the "comment end" char, that's what the +1 is for
		comment_count++; //Update comment count
	}
	return comment_count;
}

///Every line separation(\r or \n or \r\n) will be transformed into \r\n
void FixLineEndings(std::wstring &text) {
	unsigned int pos = 0;

	while (pos <= text.length()) {//length doesnt include \0
		if (text[pos] == L'\r') {
			if (text[pos + 1] != L'\n') {
				text.insert(pos + 1, L"\n");//este nunca se pasa del rango
			}
			pos += 2;
		}
		else if (text[pos] == L'\n') {
			//if(text[pos-1] == L'\r')//ya se que no tiene \r xq sino hubiera entrado por el otro
			text.insert(pos, L"\r");
			pos += 2;
		}
		else {
			pos++;
		}
	}
}