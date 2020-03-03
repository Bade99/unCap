#pragma once
#include <WinNT.h>
#include <string>

class SubtitleFunctions
{
public:
	SubtitleFunctions();
	~SubtitleFunctions();
	static void CommentRemoval(std::wstring &text, WCHAR start, WCHAR end);
};

