#ifndef _RIF_RIF_h
#define _RIF_RIF_h

#include "globals.h"

class ImageCtrl2 : public ImageCtrl {
private:
	Color m_clrSeparator;
	bool m_bShowSeparator;
	
	bool m_bInDrag;
	Point m_ptLast;
	bool m_bUseCW;
	int m_nCWDelta;
	float m_fCWPercent;
	
public:
	void SetSeparatorColor(Color clrSet) { m_clrSeparator = clrSet; Refresh(); };
	void SetShowSeparator(bool bSet) { m_bShowSeparator = bSet; Refresh(); };
	Color GetSeparatorColor() { return m_clrSeparator; };
	bool GetShowSeparator() { return m_bShowSeparator; };
	
	void SetUseCW(bool bSet) { m_bUseCW = bSet; };
	void SetCWPercent(float fSet) { m_fCWPercent = fSet; };
	
	void CapCWDelta();

	virtual void LeftDown(Point p, dword keyflags);
	virtual void LeftUp(Point p, dword keyflags);
	virtual void MouseMove(Point p, dword keyflags);
	virtual void MouseLeave();
	virtual void Paint(Draw& w);
	
	ImageCtrl2();
};

struct DisplayFile : public Display
{
	virtual void Paint(Draw& w, const Rect& r, const Value& q, Color ink, Color paper, dword style)	const;
};

struct DisplayFileStatus : public Display
{
	virtual void Paint(Draw& w, const Rect& r, const Value& q, Color ink, Color paper, dword style)	const;
};

class RIFSimpleViewer : public WithPreviewLayout<TopWindow>
{
private:
	virtual void Layout();
	
	MenuBar m_ctrlMenu;
	StatusBar m_ctrlStatus;
	StaticRect m_ctrlBackground;
	ImageCtrl2 m_actrlViewers[2];
	eColorPickerMode m_eColorPickMode;
	ColorPopUp m_ctrlColorPick;
	bool m_bEnlarge;
	
	Image m_aImages[2];
	FileSel m_ctrlFS;
	
	rif_image_desc m_aDescs[2];
	String m_astrFilenames[2];
	String m_astrStatuses[2];
	
	Size m_asizePrev[2];
	bool m_bForceRedraw[2];
	
	void MainMenu(Bar& ctrlBar);
	void FileMenu(Bar& ctrlBar);
	void ViewMenu(Bar& ctrlBar);
	
	void SelectBackgroundColor();
	void SelectSeparatorColor();
	void ShowHideSeparator();
	void ToggleEnlarge();
	void CompareOpen(int nIndex, unsigned int dwFlags);
	void CompareSwap();
	void CompareClear(int nIndex);
	
	bool RescaleImageByIndex(int nIndex, bool bUseMainSize);
	Image CreateSolidImage(Color clrFill, int nWidth, int nHeight);
	void UpdateStatus();
			
public:
	typedef RIFSimpleViewer CLASSNAME;
	RIFSimpleViewer();
	
	bool LoadImageByIndex(int nIndex, const String& strFilename, unsigned int dwFlags);
	void UpdateTitle();
	void ClearImageByIndex(int nIndex) { m_aImages[nIndex].Clear(); m_actrlViewers[nIndex].SetImage(Null); };
};

class RIFSimpleDeviceSel : public WithDeviceSelLayout<TopWindow>
{
private:
	Thread m_thrInitBackend;
	Thread m_thrQueryFilterParams;
	Vector<String> m_astrDevices;
	String m_strBackendMsg;
	std::atomic<eGenericResult> m_eBackendResult;
	
	void InitBackend();
	bool DoneWithBackend();
	
public:
	int GetDeviceIndex() { return m_ctrlDevices.GetIndex(); };
	typedef RIFSimpleDeviceSel CLASSNAME;
	RIFSimpleDeviceSel();
	
	void ShowControls(eBackendInitState eState);
};

class RIFSimpleBatchConv : public WithBatchConvLayout<TopWindow>
{
private:
	DisplayFileStatus m_dispFileAndStatus;
	Thread m_thrConvertAll;
	std::atomic<eGenericResult> m_eConvertResult;
	FilterDesc* m_pFilterDesc;
	
	void ConvertStart();
	void ConvertStop();
	void ConvertEnd();
	void SetItemStatusInThread(int nIndex, const String& strStatus, const String& strMsg, bool bUpdateProgress);
	bool ConvertInThread(int nIndex, String& strMsg, ArrayMap<String, RIF_PerfStatWrapper>& aPerfStats);
	void ShowPreview();
	
public:
	void Reset();
	void FillFileList(ArrayCtrl& ctrlSource);
	void SetFilterDesc(FilterDesc* pDesc) { m_pFilterDesc = pDesc; };
	
	typedef RIFSimpleBatchConv CLASSNAME;
	RIFSimpleBatchConv();
};

class RIFSimple : public WithMainLayout<TopWindow>
{
private:
	TimeCallback m_tcInitBackend;
	Thread m_thrConvertSelected;
	TimeCallback m_tcConvertSelectedCB;
		
	EditString m_ctrlSingleParam;
	DisplayFile m_dispFile;
	VectorMap<String, FilterDesc*> m_mapFilterDesc;
	
	MenuBar m_ctrlMenu;
	StatusBar m_ctrlStatus;
	FileSel m_ctrlFS;
		
	void FileMenu(Bar& ctrlBar);
	void DeviceMenu(Bar& ctrlBar);
	void HelpMenu(Bar& ctrlBar);
	void MainMenu(Bar& ctrlBar);
	
	void AddDirectory();
	void AddFiles();
	void RemoveFiles();
	void RemoveAllFiles();
	void SelectFilter();
	
	void ShowPreviewDblClick();
	void ShowPreview(const String& strFilename1);
	void About();
	void FillFilterMapParams(FilterDesc* pDesc);
	void ConvertSelected();
	bool ConvertSelectedInThread(String& strMsg);
	void ConvertAll();
	void EnableControls(bool bEnable);

public:
	typedef RIFSimple CLASSNAME;
	RIFSimple();
	
	void DestroyFilterMap();
	void InitFilterMap(bool bGuiLock);
	bool IsFilterMapInitted();
	const String GetOutPattern() { return ~m_ctrlOutPattern; }
};

#endif
