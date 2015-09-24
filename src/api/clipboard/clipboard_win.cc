#include "clipboard_win.h"

#include "ui/views/win/hwnd_util.h"
#include "content/nw/src/browser/native_window_aura.h"

#include <Windows.h>
#include <ShlObj.h>
#include <string>
#include <locale>
#include <codecvt>

using namespace std;

void startDnD(vector<wstring>);

wstring s2ws(const std::string& str)
{
	typedef std::codecvt_utf8<wchar_t> convert_typeX;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

string ws2s(const std::wstring& wstr)
{
	typedef std::codecvt_utf8<wchar_t> convert_typeX;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

void DoDragAndDropWin32(vector<string> files,
	content::Shell* shell)
{
	vector<wstring> wfiles;

	for (string file : files) {
		wfiles.push_back(s2ws(file));
	}

	startDnD(wfiles);
}

HWND hwnd = NULL;
HWND mainWindow = NULL;
HHOOK mousehook = NULL;
BOOL wndClassRegistered = FALSE;
BOOL wndIsOpen = FALSE;
wstring wndMessage;

void drawBorder(HDC hdc, RECT textSize, int padding) {
	HPEN hOldPen;
	COLORREF qLineColor = RGB(0xCC, 0xCC, 0xCC);
	HPEN hLinePen = CreatePen(PS_SOLID, 1, qLineColor);

	int right = textSize.right - 6 + padding * 2;
	int bottom = textSize.bottom - 4 + padding * 2 - 2;

	hOldPen = (HPEN)SelectObject(hdc, hLinePen);

	MoveToEx(hdc, 0, 0, NULL);
	LineTo(hdc, right, 0);
	LineTo(hdc, right, bottom);
	LineTo(hdc, 0, bottom);
	LineTo(hdc, 0, 0);

	SelectObject(hdc, hOldPen);
	DeleteObject(hLinePen);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	
	RECT textSize = { 0, 0, 0, 0 };
	HFONT hFont = NULL;
	int maxWidth = 120;
	int padding = 5;

	switch (msg)
	{
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		//PostQuitMessage(0);
		break;
	case WM_ERASEBKGND:
		RECT rect;
		GetClientRect(hwnd, &rect);
		FillRect((HDC)wParam, &rect, CreateSolidBrush(RGB(255, 255, 255)));
		break;
	case WM_PAINT:
		PAINTSTRUCT ps;
		HDC hdc;
		hdc = BeginPaint(hwnd, &ps);

		// Setup text style
		SetBkMode(hdc, TRANSPARENT); // Transparent background
		SetTextColor(hdc, RGB(0x70, 0x6f, 0x6f));
		hFont = CreateFont(-MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0,
			0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, L"Segoe UI");
		SelectObject(hdc, hFont);

		DrawText(hdc, wndMessage.c_str(), wndMessage.length(), &textSize,
			DT_CALCRECT);
		SetWindowPos(hwnd, HWND_TOP, textSize.left, textSize.top,
			textSize.right + padding * 2, textSize.bottom + padding * 2,
			SWP_NOMOVE | SWP_NOACTIVATE);
		textSize.left += padding;
		textSize.top += padding;
		textSize.right += padding;
		textSize.bottom += padding;
		DrawText(hdc, wndMessage.c_str(), wndMessage.length(), &textSize, NULL);

		drawBorder(hdc, textSize, padding);

		EndPaint(hwnd, &ps);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

void closeNotificationWindow() {
	UnhookWindowsHookEx(mousehook);
	DestroyWindow(hwnd);
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0) {

		MOUSEHOOKSTRUCT* pMouseStruct = (MOUSEHOOKSTRUCT*)lParam;
		if (NULL != pMouseStruct) {

			RECT wndRect;
			GetWindowRect(mainWindow, &wndRect);

			POINT& mouseLocation = pMouseStruct->pt;
			BOOL isInside = PtInRect(&wndRect, mouseLocation);
			if (isInside) {
				if (wndIsOpen) {
					ShowWindow(hwnd, SW_HIDE);
					wndIsOpen = FALSE;
				}
			} else {
				if (!wndIsOpen) {
					ShowWindow(hwnd, SW_SHOWNOACTIVATE);
					wndIsOpen = TRUE;
				}

				RECT notiWndRect;
				GetWindowRect(hwnd, &notiWndRect);
				SIZE notiwndSize = {
					notiWndRect.right - notiWndRect.left,
					notiWndRect.bottom - notiWndRect.top };

				SetWindowPos(hwnd, HWND_TOP, mouseLocation.x + 3,
					mouseLocation.y - notiwndSize.cy - 3, 0, 0,
					SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
			}

			if (GetAsyncKeyState(VK_SHIFT) || !GetAsyncKeyState(VK_LBUTTON)) {
				UnhookWindowsHookEx(mousehook);
				DestroyWindow(hwnd);
			}
		}
	}
	return CallNextHookEx(0, nCode, wParam, lParam);
}

void registerWndClass(wstring className) {
	if (wndClassRegistered) {
		return;
	}

	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = className.c_str();
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Window Registration Failed!", L"Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	wndClassRegistered = TRUE;
}

void registerNotificationWindow(std::string message_,
	content::Shell* shell) {

	//mainWindow = views::HWNDForNativeWindow(shell->GetNativeWIndow);
	gfx::NativeView nativeView = static_cast<nw::NativeWindowAura*>(
		shell->window())->GetHostView();

	mainWindow = views::HWNDForNativeView(nativeView);

	wndMessage = s2ws(message_);
	createNotificationWindow();
}

void createNotificationWindow() {
	const wstring kWndClassName = L"Notifiction_Window";

	registerWndClass(kWndClassName);

	DWORD dwExStyle = WS_EX_COMPOSITED | WS_EX_LAYERED | WS_EX_NOACTIVATE |
		WS_EX_TOPMOST | WS_EX_TRANSPARENT;
	DWORD dwStyle = WS_POPUP;

	// Step 2: Creating the Window
	hwnd = CreateWindowEx(
		dwExStyle,
		kWndClassName.c_str(),
		L"",
		dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, 100, 17,
		NULL, NULL, GetModuleHandle(NULL), NULL);

	if (hwnd == NULL)
	{
		MessageBox(NULL, L"Window Creation Failed!", L"Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	// SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE |
	//	SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	ShowWindow(hwnd, SW_HIDE);
	UpdateWindow(hwnd);

	COLORREF RRR = RGB(0, 0, 0);
	SetLayeredWindowAttributes(hwnd, RRR, (BYTE)229, LWA_ALPHA);

	mousehook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);

}


HGLOBAL create_drop_files(vector<wstring> files)
{
	size_t size = 0;
	for (auto file : files) {
		// +1 = null terminator
		size += (file.length() + 1) * sizeof(wchar_t);
	}

	if (files.empty()) {
		return NULL;
	}

	HGLOBAL hMem = GlobalAlloc(GHND, sizeof(DROPFILES) + size + 2);
	if (hMem == NULL) {
		return NULL;
	}

	DROPFILES* dfiles = (DROPFILES*)GlobalLock(hMem);
	if (dfiles == NULL) {
		GlobalFree(hMem);
		return NULL;
	}

	dfiles->pFiles = sizeof(DROPFILES);
	GetCursorPos(&(dfiles->pt));
	dfiles->fNC = TRUE;
	dfiles->fWide = TRUE;

	char* pFile = (char*)&dfiles[1];
	for (auto i = 0; i < files.size(); ++i) {
		wstring& fileDir = files[i];

		size = (fileDir.length() + 1) * sizeof(wchar_t);

		memcpy(pFile, fileDir.c_str(), size);
		pFile += size;
	}

	GlobalUnlock(hMem);
	return hMem;
}

void free_drop_files(HGLOBAL hMem)
{
	GlobalFree(hMem);
}

HRESULT CreateDropSource(IDropSource **ppDropSource);
HRESULT CreateDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmeds, UINT count, IDataObject **ppDataObject);

DWORD WINAPI handle_dnd(HGLOBAL drop_files)
{
	IDataObject* pDataObject;
	IDropSource* pDropSource;
	DWORD		 dwEffect;
	DWORD		 dwResult;

	FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stgmed = { TYMED_HGLOBAL, { 0 }, 0 };
	stgmed.hGlobal = drop_files;

	CreateDropSource(&pDropSource);
	CreateDataObject(&fmtetc, &stgmed, 1, &pDataObject);

	dwResult = DoDragDrop(pDataObject, pDropSource, DROPEFFECT_COPY, &dwEffect);

	pDataObject->Release();
	pDropSource->Release();

	return 0;
}

void startDnD(vector<wstring> files) {
	HGLOBAL dndFiles = create_drop_files(files);

	const HCURSOR cursorNo = LoadCursor(GetModuleHandle(NULL), IDC_CROSS);
	SetCursor(cursorNo);

	handle_dnd(dndFiles);
	free_drop_files(dndFiles);
}

// defined in enumformat.cc
HRESULT CreateEnumFormatEtc(UINT nNumFormats, FORMATETC *pFormatEtc, IEnumFORMATETC **ppEnumFormatEtc);

// dataobject.cc
class CDataObject : public IDataObject
{
public:
	//
	// IUnknown members
	//
	HRESULT __stdcall QueryInterface(REFIID iid, void ** ppvObject);
	ULONG   __stdcall AddRef(void);
	ULONG   __stdcall Release(void);

	//
	// IDataObject members
	//
	HRESULT __stdcall GetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium);
	HRESULT __stdcall GetDataHere(FORMATETC *pFormatEtc, STGMEDIUM *pMedium);
	HRESULT __stdcall QueryGetData(FORMATETC *pFormatEtc);
	HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC *pFormatEct, FORMATETC *pFormatEtcOut);
	HRESULT __stdcall SetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium, BOOL fRelease);
	HRESULT __stdcall EnumFormatEtc(DWORD      dwDirection, IEnumFORMATETC **ppEnumFormatEtc);
	HRESULT __stdcall DAdvise(FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
	HRESULT __stdcall DUnadvise(DWORD      dwConnection);
	HRESULT __stdcall EnumDAdvise(IEnumSTATDATA **ppEnumAdvise);

	//
	// Constructor / Destructor
	//
	CDataObject(FORMATETC *fmt, STGMEDIUM *stgmed, int count);
	~CDataObject();

private:

	int LookupFormatEtc(FORMATETC *pFormatEtc);

	//
	// any private members and functions
	//
	LONG	   m_lRefCount;

	FORMATETC *m_pFormatEtc;
	STGMEDIUM *m_pStgMedium;
	LONG	   m_nNumFormats;

};

//
//	Constructor
//
CDataObject::CDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmed, int count)
{
	m_lRefCount = 1;
	m_nNumFormats = count;

	m_pFormatEtc = new FORMATETC[count];
	m_pStgMedium = new STGMEDIUM[count];

	for (int i = 0; i < count; i++)
	{
		m_pFormatEtc[i] = fmtetc[i];
		m_pStgMedium[i] = stgmed[i];
	}
}

//
//	Destructor
//
CDataObject::~CDataObject()
{
	// cleanup
	if (m_pFormatEtc) delete[] m_pFormatEtc;
	if (m_pStgMedium) delete[] m_pStgMedium;

	OutputDebugString(L"oof\n");
}

//
//	IUnknown::AddRef
//
ULONG __stdcall CDataObject::AddRef(void)
{
	// increment object reference count
	return InterlockedIncrement(&m_lRefCount);
}

//
//	IUnknown::Release
//
ULONG __stdcall CDataObject::Release(void)
{
	// decrement object reference count
	LONG count = InterlockedDecrement(&m_lRefCount);

	if (count == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return count;
	}
}

//
//	IUnknown::QueryInterface
//
HRESULT __stdcall CDataObject::QueryInterface(REFIID iid, void **ppvObject)
{
	// check to see what interface has been requested
	if (iid == IID_IDataObject || iid == IID_IUnknown)
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}
	else
	{
		*ppvObject = 0;
		return E_NOINTERFACE;
	}
}

HGLOBAL DupMem(HGLOBAL hMem)
{
	// lock the source memory object
	DWORD   len = GlobalSize(hMem);
	PVOID   source = GlobalLock(hMem);

	// create a fixed "global" block - i.e. just
	// a regular lump of our process heap
	PVOID   dest = GlobalAlloc(GMEM_FIXED, len);

	memcpy(dest, source, len);

	GlobalUnlock(hMem);

	return dest;
}

int CDataObject::LookupFormatEtc(FORMATETC *pFormatEtc)
{
	for (int i = 0; i < m_nNumFormats; i++)
	{
		if ((pFormatEtc->tymed    &  m_pFormatEtc[i].tymed) &&
			pFormatEtc->cfFormat == m_pFormatEtc[i].cfFormat &&
			pFormatEtc->dwAspect == m_pFormatEtc[i].dwAspect)
		{
			return i;
		}
	}

	return -1;

}

//
//	IDataObject::GetData
//
HRESULT __stdcall CDataObject::GetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
{
	int idx;

	//
	// try to match the requested FORMATETC with one of our supported formats
	//
	if ((idx = LookupFormatEtc(pFormatEtc)) == -1)
	{
		return DV_E_FORMATETC;
	}

	//
	// found a match! transfer the data into the supplied storage-medium
	//
	pMedium->tymed = m_pFormatEtc[idx].tymed;
	pMedium->pUnkForRelease = 0;

	switch (m_pFormatEtc[idx].tymed)
	{
	case TYMED_HGLOBAL:

		pMedium->hGlobal = DupMem(m_pStgMedium[idx].hGlobal);
		//return S_OK;
		break;

	default:
		return DV_E_FORMATETC;
	}

	return S_OK;
}

//
//	IDataObject::GetDataHere
//
HRESULT __stdcall CDataObject::GetDataHere(FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
{
	// GetDataHere is only required for IStream and IStorage mediums
	// It is an error to call GetDataHere for things like HGLOBAL and other clipboard formats
	//
	//	OleFlushClipboard 
	//
	return DATA_E_FORMATETC;
}

//
//	IDataObject::QueryGetData
//
//	Called to see if the IDataObject supports the specified format of data
//
HRESULT __stdcall CDataObject::QueryGetData(FORMATETC *pFormatEtc)
{
	return (LookupFormatEtc(pFormatEtc) == -1) ? DV_E_FORMATETC : S_OK;
}

//
//	IDataObject::GetCanonicalFormatEtc
//
HRESULT __stdcall CDataObject::GetCanonicalFormatEtc(FORMATETC *pFormatEct, FORMATETC *pFormatEtcOut)
{
	// Apparently we have to set this field to NULL even though we don't do anything else
	pFormatEtcOut->ptd = NULL;
	return E_NOTIMPL;
}

//
//	IDataObject::SetData
//
HRESULT __stdcall CDataObject::SetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium, BOOL fRelease)
{
	return E_NOTIMPL;
}

//
//	IDataObject::EnumFormatEtc
//
HRESULT __stdcall CDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc)
{
	if (dwDirection == DATADIR_GET)
	{
		// for Win2k+ you can use the SHCreateStdEnumFmtEtc API call, however
		// to support all Windows platforms we need to implement IEnumFormatEtc ourselves.
		return CreateEnumFormatEtc(m_nNumFormats, m_pFormatEtc, ppEnumFormatEtc);
	}
	else
	{
		// the direction specified is not support for drag+drop
		return E_NOTIMPL;
	}
}

//
//	IDataObject::DAdvise
//
HRESULT __stdcall CDataObject::DAdvise(FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//
//	IDataObject::DUnadvise
//
HRESULT __stdcall CDataObject::DUnadvise(DWORD dwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//
//	IDataObject::EnumDAdvise
//
HRESULT __stdcall CDataObject::EnumDAdvise(IEnumSTATDATA **ppEnumAdvise)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//
//	Helper function
//
HRESULT CreateDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmeds, UINT count, IDataObject **ppDataObject)
{
	if (ppDataObject == 0)
		return E_INVALIDARG;

	*ppDataObject = new CDataObject(fmtetc, stgmeds, count);

	return (*ppDataObject) ? S_OK : E_OUTOFMEMORY;
}


// dropsource.cc

class CDropSource : public IDropSource
{
public:
	//
	// IUnknown members
	//
	HRESULT __stdcall QueryInterface(REFIID iid, void ** ppvObject);
	ULONG   __stdcall AddRef(void);
	ULONG   __stdcall Release(void);

	//
	// IDropSource members
	//
	HRESULT __stdcall QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
	HRESULT __stdcall GiveFeedback(DWORD dwEffect);

	//
	// Constructor / Destructor
	//
	CDropSource();
	~CDropSource();

private:

	//
	// private members and functions
	//
	LONG	   m_lRefCount;
};

//
//	Constructor
//
CDropSource::CDropSource()
{
	m_lRefCount = 1;
}

//
//	Destructor
//
CDropSource::~CDropSource()
{
}

//
//	IUnknown::AddRef
//
ULONG __stdcall CDropSource::AddRef(void)
{
	// increment object reference count
	return InterlockedIncrement(&m_lRefCount);
}

//
//	IUnknown::Release
//
ULONG __stdcall CDropSource::Release(void)
{
	// decrement object reference count
	LONG count = InterlockedDecrement(&m_lRefCount);

	if (count == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return count;
	}
}

//
//	IUnknown::QueryInterface
//
HRESULT __stdcall CDropSource::QueryInterface(REFIID iid, void **ppvObject)
{
	// check to see what interface has been requested
	if (iid == IID_IDropSource || iid == IID_IUnknown)
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}
	else
	{
		*ppvObject = 0;
		return E_NOINTERFACE;
	}
}

//
//	CDropSource::QueryContinueDrag
//
//	Called by OLE whenever Escape/Control/Shift/Mouse buttons have changed
//
HRESULT __stdcall CDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	// if the <Escape> key has been pressed since the last call, cancel the drop
	if (fEscapePressed == TRUE)
		return DRAGDROP_S_CANCEL;

	// if the <LeftMouse> button has been released, then do the drop!
	if ((grfKeyState & MK_LBUTTON) == 0)
		return DRAGDROP_S_DROP;

	// continue with the drag-drop
	return S_OK;
}

//
//	CDropSource::GiveFeedback
//
//	Return either S_OK, or DRAGDROP_S_USEDEFAULTCURSORS to instruct OLE to use the
//  default mouse cursor images
//
HRESULT __stdcall CDropSource::GiveFeedback(DWORD dwEffect)
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

//
//	Helper routine to create an IDropSource object
//	
HRESULT CreateDropSource(IDropSource **ppDropSource)
{
	if (ppDropSource == 0)
		return E_INVALIDARG;

	*ppDropSource = new CDropSource();

	return (*ppDropSource) ? S_OK : E_OUTOFMEMORY;

}

// enumformat.cc

class CEnumFormatEtc : public IEnumFORMATETC
{
public:

	//
	// IUnknown members
	//
	HRESULT __stdcall  QueryInterface(REFIID iid, void ** ppvObject);
	ULONG	__stdcall  AddRef(void);
	ULONG	__stdcall  Release(void);

	//
	// IEnumFormatEtc members
	//
	HRESULT __stdcall  Next(ULONG celt, FORMATETC * rgelt, ULONG * pceltFetched);
	HRESULT __stdcall  Skip(ULONG celt);
	HRESULT __stdcall  Reset(void);
	HRESULT __stdcall  Clone(IEnumFORMATETC ** ppEnumFormatEtc);

	//
	// Construction / Destruction
	//
	CEnumFormatEtc(FORMATETC *pFormatEtc, int nNumFormats);
	~CEnumFormatEtc();

private:

	LONG		m_lRefCount;		// Reference count for this COM interface
	ULONG		m_nIndex;			// current enumerator index
	ULONG		m_nNumFormats;		// number of FORMATETC members
	FORMATETC * m_pFormatEtc;		// array of FORMATETC objects
};

//
//	"Drop-in" replacement for SHCreateStdEnumFmtEtc. Called by CDataObject::EnumFormatEtc
//
HRESULT CreateEnumFormatEtc(UINT nNumFormats, FORMATETC *pFormatEtc, IEnumFORMATETC **ppEnumFormatEtc)
{
	if (nNumFormats == 0 || pFormatEtc == 0 || ppEnumFormatEtc == 0)
		return E_INVALIDARG;

	*ppEnumFormatEtc = new CEnumFormatEtc(pFormatEtc, nNumFormats);

	return (*ppEnumFormatEtc) ? S_OK : E_OUTOFMEMORY;
}

//
//	Helper function to perform a "deep" copy of a FORMATETC
//
static void DeepCopyFormatEtc(FORMATETC *dest, FORMATETC *source)
{
	// copy the source FORMATETC into dest
	*dest = *source;

	if (source->ptd)
	{
		// allocate memory for the DVTARGETDEVICE if necessary
		dest->ptd = (DVTARGETDEVICE*)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));

		// copy the contents of the source DVTARGETDEVICE into dest->ptd
		*(dest->ptd) = *(source->ptd);
	}
}

//
//	Constructor 
//
CEnumFormatEtc::CEnumFormatEtc(FORMATETC *pFormatEtc, int nNumFormats)
{
	m_lRefCount = 1;
	m_nIndex = 0;
	m_nNumFormats = nNumFormats;
	m_pFormatEtc = new FORMATETC[nNumFormats];

	// copy the FORMATETC structures
	for (int i = 0; i < nNumFormats; i++)
	{
		DeepCopyFormatEtc(&m_pFormatEtc[i], &pFormatEtc[i]);
	}
}

//
//	Destructor
//
CEnumFormatEtc::~CEnumFormatEtc()
{
	if (m_pFormatEtc)
	{
		for (ULONG i = 0; i < m_nNumFormats; i++)
		{
			if (m_pFormatEtc[i].ptd)
				CoTaskMemFree(m_pFormatEtc[i].ptd);
		}

		delete[] m_pFormatEtc;
	}
}

//
//	IUnknown::AddRef
//
ULONG __stdcall CEnumFormatEtc::AddRef(void)
{
	// increment object reference count
	return InterlockedIncrement(&m_lRefCount);
}

//
//	IUnknown::Release
//
ULONG __stdcall CEnumFormatEtc::Release(void)
{
	// decrement object reference count
	LONG count = InterlockedDecrement(&m_lRefCount);

	if (count == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return count;
	}
}

//
//	IUnknown::QueryInterface
//
HRESULT __stdcall CEnumFormatEtc::QueryInterface(REFIID iid, void **ppvObject)
{
	// check to see what interface has been requested
	if (iid == IID_IEnumFORMATETC || iid == IID_IUnknown)
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}
	else
	{
		*ppvObject = 0;
		return E_NOINTERFACE;
	}
}

//
//	IEnumFORMATETC::Next
//
//	If the returned FORMATETC structure contains a non-null "ptd" member, then
//  the caller must free this using CoTaskMemFree (stated in the COM documentation)
//
HRESULT __stdcall CEnumFormatEtc::Next(ULONG celt, FORMATETC *pFormatEtc, ULONG * pceltFetched)
{
	ULONG copied = 0;

	// validate arguments
	if (celt == 0 || pFormatEtc == 0)
		return E_INVALIDARG;

	// copy FORMATETC structures into caller's buffer
	while (m_nIndex < m_nNumFormats && copied < celt)
	{
		DeepCopyFormatEtc(&pFormatEtc[copied], &m_pFormatEtc[m_nIndex]);
		copied++;
		m_nIndex++;
	}

	// store result
	if (pceltFetched != 0)
		*pceltFetched = copied;

	// did we copy all that was requested?
	return (copied == celt) ? S_OK : S_FALSE;
}

//
//	IEnumFORMATETC::Skip
//
HRESULT __stdcall CEnumFormatEtc::Skip(ULONG celt)
{
	m_nIndex += celt;
	return (m_nIndex <= m_nNumFormats) ? S_OK : S_FALSE;
}

//
//	IEnumFORMATETC::Reset
//
HRESULT __stdcall CEnumFormatEtc::Reset(void)
{
	m_nIndex = 0;
	return S_OK;
}

//
//	IEnumFORMATETC::Clone
//
HRESULT __stdcall CEnumFormatEtc::Clone(IEnumFORMATETC ** ppEnumFormatEtc)
{
	HRESULT hResult;

	// make a duplicate enumerator
	hResult = CreateEnumFormatEtc(m_nNumFormats, m_pFormatEtc, ppEnumFormatEtc);

	if (hResult == S_OK)
	{
		// manually set the index state
		((CEnumFormatEtc *)*ppEnumFormatEtc)->m_nIndex = m_nIndex;
	}

	return hResult;
}
