#pragma once
#include <shobjidl.h>
#include <shlwapi.h>
#include <new>

class CDialogEventHandler : public IFileDialogEvents,
	public IFileDialogControlEvents
{
public:
	// IUnknown methods
	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		static const QITAB qit[] = {
			QITABENT(CDialogEventHandler, IFileDialogEvents),
			QITABENT(CDialogEventHandler, IFileDialogControlEvents),
		{ 0 },
#pragma warning(suppress:4838)
		};
		return QISearch(this, qit, riid, ppv);
	}

	IFACEMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&_cRef);
	}

	IFACEMETHODIMP_(ULONG) Release()
	{
		long cRef = InterlockedDecrement(&_cRef);
		if (!cRef)
			delete this;
		return cRef;
	}

	// IFileDialogEvents methods
	IFACEMETHODIMP OnFileOk(IFileDialog *pfd);
	IFACEMETHODIMP OnFolderChange(IFileDialog *) { return S_OK; };
	IFACEMETHODIMP OnFolderChanging(IFileDialog *, IShellItem *) { return S_OK; };
	IFACEMETHODIMP OnHelp(IFileDialog *) { return S_OK; };
	IFACEMETHODIMP OnSelectionChange(IFileDialog *) { return S_OK; };
	IFACEMETHODIMP OnShareViolation(IFileDialog *, IShellItem *, FDE_SHAREVIOLATION_RESPONSE *) { return S_OK; };// file being saved or replaced is in use and cannot be written to (a sharing violation)
	IFACEMETHODIMP OnTypeChange(IFileDialog *pfd);
	IFACEMETHODIMP OnOverwrite(IFileDialog *, IShellItem *, FDE_OVERWRITE_RESPONSE *) { return S_OK; };//in the save dialog, when user wants to override something

	// IFileDialogControlEvents methods
	IFACEMETHODIMP OnItemSelected(IFileDialogCustomize *pfdc, DWORD dwIDCtl, DWORD dwIDItem);
	IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize *, DWORD) { return S_OK; };
	IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize *, DWORD, BOOL) { return S_OK; };
	IFACEMETHODIMP OnControlActivating(IFileDialogCustomize *, DWORD) { return S_OK; };

	// My methods
	IFACEMETHODIMP _DisplayMessage(IFileDialog *pfd, PCWSTR pszMessage);

	CDialogEventHandler() : _cRef(1) { };
private:
	~CDialogEventHandler() { };
	long _cRef;
};

//file type index
#define INDEX_SRT 1
#define INDEX_ASS 2
#define INDEX_SSA 3
#define INDEX_TXT 4
#define INDEX_ALL 5

//extra dialog controls
#define CONTROL_GROUP           2000 //element that will contain all the other controls
#define CONTROL_RADIOBUTTONLIST 1
#define CONTROL_RADIOBUTTON1    1
#define CONTROL_RADIOBUTTON2    2 

#define OPENCHOICES 2 //this 3 would be only for an open dialog
#define OPEN 0
#define OPEN_AS_READONLY 1

// IFileDialogEvents methods
// This method gets called when the file-type is changed (combo-box selection changes).
// For sample sake, let's react to this event by changing the properties show.
HRESULT CDialogEventHandler::OnTypeChange(IFileDialog *pfd)
{
	IFileSaveDialog *pfsd;
	HRESULT hr = pfd->QueryInterface(&pfsd);
	if (SUCCEEDED(hr))
	{
		UINT uIndex;
		hr = pfsd->GetFileTypeIndex(&uIndex);   // index of current file-type
		if (SUCCEEDED(hr))
		{
			IPropertyDescriptionList *pdl = NULL;

			switch (uIndex)
			{
			case INDEX_SRT:
				// When .srt is selected, let's ask for some arbitrary property, say Title.
				//hr = PSGetPropertyDescriptionListFromString(L"prop:System.Title", IID_PPV_ARGS(&pdl));
				//if (SUCCEEDED(hr))
				//{
				//	// FALSE as second param == do not show default properties.
				//	hr = pfsd->SetCollectedProperties(pdl, FALSE);
				//	pdl->Release();
				//}
				break;

			case INDEX_ASS:
				// When .ass is selected, let's ask for some other arbitrary property, say Keywords.
				//hr = PSGetPropertyDescriptionListFromString(L"prop:System.Keywords", IID_PPV_ARGS(&pdl));
				//if (SUCCEEDED(hr))
				//{
				//	// FALSE as second param == do not show default properties.
				//	hr = pfsd->SetCollectedProperties(pdl, FALSE);
				//	pdl->Release();
				//}
				break;

			case INDEX_SSA:
				// When .ssa is selected, let's ask for some other arbitrary property, say Author.
				//hr = PSGetPropertyDescriptionListFromString(L"prop:System.Author", IID_PPV_ARGS(&pdl));
				//if (SUCCEEDED(hr))
				//{
				//	// TRUE as second param == show default properties as well, but show Author property first in list.
				//	hr = pfsd->SetCollectedProperties(pdl, TRUE);
				//	pdl->Release();
				//}
				break;
			case INDEX_TXT:
				break;
			case INDEX_ALL:
				break;
			}
		}
		pfsd->Release();
	}
	return hr;
}

// IFileDialogControlEvents
// This method gets called when an dialog control item selection happens (radio-button selection. etc).
// For sample sake, let's react to this event by changing the dialog title.
HRESULT CDialogEventHandler::OnItemSelected(IFileDialogCustomize *pfdc, DWORD dwIDCtl, DWORD dwIDItem)
{
	IFileDialog *pfd = NULL;
	HRESULT hr = pfdc->QueryInterface(&pfd);
	if (SUCCEEDED(hr))
	{
		if (dwIDCtl == CONTROL_RADIOBUTTONLIST)
		{
			switch (dwIDItem)
			{
			case CONTROL_RADIOBUTTON1:
				hr = pfd->SetTitle(L"UTF-8 Dialog");
				break;

			case CONTROL_RADIOBUTTON2:
				hr = pfd->SetTitle(L"UTF-16 Dialog");
				break;
			}
		}
		pfd->Release();
	}
	return hr;
}

HRESULT CDialogEventHandler::OnFileOk(IFileDialog *pfd)
{
	IShellItem *psiResult;
	HRESULT hr = pfd->GetResult(&psiResult);

	if (SUCCEEDED(hr))
	{
		//https://docs.microsoft.com/en-us/windows/desktop/shell/sfgao
		SFGAOF attributes;
		hr = psiResult->GetAttributes(SFGAO_CANCOPY, &attributes);//pregunto sobre ciertos atributos del archivo/folder/etc

		if (SUCCEEDED(hr))
		{
			if (attributes & SFGAO_CANCOPY)
			{
				// Accept the file.
				hr = S_OK;
			}
			else
			{
				// Refuse the file.
				hr = S_FALSE;

				_DisplayMessage(pfd, L"Not a file that can be copied");
			}
		}
		psiResult->Release();
	}
	return hr;
};

HRESULT CDialogEventHandler::_DisplayMessage(IFileDialog *pfd, PCWSTR pszMessage)
{
	IOleWindow *pWindow;
	HRESULT hr = pfd->QueryInterface(IID_PPV_ARGS(&pWindow));

	if (SUCCEEDED(hr))
	{
		HWND hwndDialog;
		hr = pWindow->GetWindow(&hwndDialog);

		if (SUCCEEDED(hr))
		{
			MessageBox(hwndDialog, pszMessage, L"", MB_OK);
		}
		pWindow->Release();
	}
	return hr;
}

// Instance creation helper
HRESULT CDialogEventHandler_CreateInstance(REFIID riid, void **ppv)
{
	*ppv = NULL;
	CDialogEventHandler *pDialogEventHandler = new (std::nothrow) CDialogEventHandler();
	HRESULT hr = pDialogEventHandler ? S_OK : E_OUTOFMEMORY;
	if (SUCCEEDED(hr))
	{
		hr = pDialogEventHandler->QueryInterface(riid, ppv);
		pDialogEventHandler->Release();
	}
	return hr;
}