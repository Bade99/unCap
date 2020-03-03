#include "SubtitleFunctions.h"



SubtitleFunctions::SubtitleFunctions()
{
}


SubtitleFunctions::~SubtitleFunctions()
{
}

void SubtitleFunctions::CommentRemoval(std::wstring &text, WCHAR start, WCHAR end) {
	//TODO(fran):use more than 1 char?

	size_t start_pos = 0, end_pos = 0;

	while (true) {
		start_pos = text.find(start, start_pos);
		end_pos = text.find(end, start_pos + 1);//+1, dont add it here 'cause breaks the if when end_pos=-1(string::npos)
		if (start_pos == std::string::npos || end_pos == std::string::npos) {
			//SendMessage(hRemoveProgress, PBM_SETPOS, text_length, 0);
			break;
		}
		text.erase(text.begin() + start_pos, text.begin() + end_pos + 1); //@@need to start handling exceptions
	}
}
