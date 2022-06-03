#include "RIF.h"
#include "backend.h"

RIFSimple* g_pMainForm = nullptr;
RIFSimpleViewer* g_pViewerForm = nullptr;
RIFSimpleBatchConv* g_pBatchForm = nullptr;
RIFSimpleDeviceSel* g_pDeviceForm = nullptr;

void ImageCtrl2::Paint(Draw& w)
{
	if(!img) return;
	
	Size sz = GetSize();
	Size bsz = GetStdSize();
		
	if (!m_bUseCW)
	{
		w.DrawImage((sz.cx - bsz.cx) / 2, (sz.cy - bsz.cy) / 2, img);
	}
	else
	{
		if (m_fCWPercent != -1.0f)
		{
			m_nCWDelta = (float)GetStdSize().cx * m_fCWPercent;
			m_fCWPercent = -1.0f;
		}
		
		Rect rcSrc = Rect(0, 0, m_nCWDelta, bsz.cy);
		w.DrawImage((sz.cx - bsz.cx) / 2, (sz.cy - bsz.cy) / 2, img, rcSrc);
		
		if (m_bShowSeparator)
		{
			CapCWDelta();
			int nX = (sz.cx - bsz.cx) / 2 + m_nCWDelta;
			w.DrawLine(nX, 0, nX, sz.cy, 2, m_clrSeparator);
		}
	}
}

void ImageCtrl2::CapCWDelta()
{
	if (m_nCWDelta < 0)
		m_nCWDelta = 0;
	
	int nWidth = GetStdSize().cx;
	if (m_nCWDelta > nWidth)
		m_nCWDelta = nWidth;
}

void ImageCtrl2::LeftDown(Point p, dword keyflags)
{
	if (m_bUseCW)
	{
		m_bInDrag = true;
		m_ptLast = p;
	}
}

void ImageCtrl2::LeftUp(Point p, dword keyflags)
{
	m_bInDrag = false;
}

void ImageCtrl2::MouseMove(Point p, dword keyflags)
{
	if (m_bUseCW && m_bInDrag)
	{
		m_nCWDelta += (p.x - m_ptLast.x);
		CapCWDelta();
		
		m_ptLast = p;
		Refresh();
	}
}

void ImageCtrl2::MouseLeave()
{
	m_bInDrag = false;
}

ImageCtrl2::ImageCtrl2()
{
	m_bUseCW = false;
	m_bShowSeparator = true;
	m_clrSeparator = LtRed();
	m_nCWDelta = 0;
	
	ImageCtrl();
}

void DisplayFile::Paint(Draw& w, const Rect& r, const Value& q, Color ink, Color paper, dword style) const
{
	PaintBackground(w, r, q, ink, paper, style);
	
	Font font = StdFont();
	
	Image img = CtrlImg::File();
	Size sizeImage = img.GetSize();
	sizeImage.cy = ((r.bottom - r.top) >> 1) - (sizeImage.cy >> 1);
	
	w.DrawImage(r.left, r.top + sizeImage.cy, img);
	w.DrawText(r.left + sizeImage.cx + 1, r.top, ~q, font, ink);
}

void DisplayFileStatus::Paint(Draw& w, const Rect& r, const Value& q, Color ink, Color paper, dword style) const
{
	PaintBackground(w, r, q, ink, paper, style);
	
	Image img = CtrlImg::File();
	Font font = StdFont();
	
	if (q == g_strFileStatusInQueue)
		img = CtrlImg::menu_radio();
	else if (q == g_strFileStatusInProgress)
	{
		font = StdFont().Bold();
		img = CtrlImg::right_arrow();
	}
	else if (q == g_strFileStatusDone)
		img = CtrlImg::menu_check();
	else
		img = CtrlImg::Remove();
	
	Size sizeImage = img.GetSize();
	sizeImage.cy = ((r.bottom - r.top) >> 1) - (sizeImage.cy >> 1);
	
	w.DrawImage(r.left, r.top + sizeImage.cy, img);
	w.DrawText(r.left + sizeImage.cx + 1, r.top, ~q, font, ink);
}

RIFSimple::RIFSimple()
{
	g_pMainForm = this;
	
	m_tcInitBackend.KillSet(0, [=]
	{
		g_pDeviceForm->ShowControls(BIS_BACKEND);
		g_pDeviceForm->Execute();
		EnableControls(true);
	});
	
	WhenClose = [=]
	{
		DestroyFilterMap();
		ShutdownThreads();
		RIF_DeleteObjects();
		Close();
	};
	
	CtrlLayout(*this, Format(g_strMainWindowPattern, g_strAPI));
	Zoomable(true).Sizeable(true);
	
	AddFrame(m_ctrlMenu);
	AddFrame(m_ctrlStatus);
		
	m_ctrlFilePattern <<= g_AppConfig.strFilePattern;
	m_ctrlOutPattern <<= g_AppConfig.strOutPattern;
	
	m_ctrlAddDir << [=] { AddDirectory(); };
	m_ctrlAddFiles << [=] { AddFiles(); };
	m_ctrlRemoveFiles << [=] { RemoveFiles(); };
	m_ctrlRemoveAllFiles << [=] { RemoveAllFiles(); };
	
	m_ctrlConvertSel << [=] { ConvertSelected(); };
	m_ctrlConvertAll << [=] { ConvertAll(); };
	
	m_ctrlMenu.Set( [=] (Bar& b) { MainMenu(b); } );
	
	m_ctrlStatus = t_(g_strStatusReady);
	m_ctrlFS.ActiveDir(GetExeFolder());
	
	m_ctrlFileList.WhenLeftDouble = [=] { ShowPreviewDblClick(); };
	m_ctrlFileList.AddColumn().SetDisplay(m_dispFile);
		
	m_ctrlFilter << [=] { SelectFilter(); };
	
	HeaderCtrl::Column& ctrlH1 = m_ctrlParams.AddColumn(g_strParameter).HeaderTab();
	ctrlH1.SetRatio(0.15).Hide();
	ctrlH1.Show();
	
	HeaderCtrl::Column& ctrlH2 = m_ctrlParams.AddColumn(g_strType).HeaderTab();
	ctrlH2.SetRatio(0.15).Hide();
	ctrlH2.Show();
	
	HeaderCtrl::Column& ctrlH3 = m_ctrlParams.AddColumn(g_strValue).Edit(m_ctrlSingleParam).HeaderTab();
	ctrlH3.SetRatio(0.35).Hide();
	ctrlH3.Show();
	
	HeaderCtrl::Column& ctrlH4 = m_ctrlParams.AddColumn(g_strDescription).HeaderTab();
	ctrlH4.SetRatio(0.35).Hide();
	ctrlH4.Show();
	
	EnableControls(false);
}

void RIFSimple::EnableControls(bool bEnable)
{
	m_ctrlConvertSel.Enable(bEnable);
	m_ctrlConvertAll.Enable(bEnable);
	m_ctrlAddFiles.Enable(bEnable);
	m_ctrlAddDir.Enable(bEnable);
	m_ctrlRemoveFiles.Enable(bEnable);
	m_ctrlRemoveAllFiles.Enable(bEnable);
	m_ctrlMenu.Enable(bEnable);
	m_ctrlFilter.Enable(bEnable);
	m_ctrlParams.Enable(bEnable);
	m_ctrlFilePattern.Enable(bEnable);
	m_ctrlOutPattern.Enable(bEnable);
}

bool RIFSimple::IsFilterMapInitted()
{
	return m_mapFilterDesc.GetCount() > 0;
}

void RIFSimple::InitFilterMap(bool bGuiLock)
{
	for(const KeyValueRef<String, AppConfig::Filter>& kvrPair : ~g_AppConfig.mapFilters)
	{
		FilterDesc* pItem = new FilterDesc();
		
		for (int i = 0; i < kvrPair.value.anCommands.GetCount(); i++)
			pItem->m_adwCommands.Add(kvrPair.value.anCommands[i]);
		
		for (int i = 0; i < kvrPair.value.anComputeType.GetCount(); i++)
			pItem->m_adwComputeTypes.Add(kvrPair.value.anComputeType[i]);
		
		for (int i = 0; i < kvrPair.value.anSetParamsFunc.GetCount(); i++)
			pItem->m_apSetParamsFuncs.Add(g_aSetFilterParamsFuncs[kvrPair.value.anSetParamsFunc[i]]);
			
		pItem->m_pExecuteFunc = g_aExecuteFilterFuncs[kvrPair.value.nExecuteFunc];
		
		for (int i = 0; i < kvrPair.value.anCreateImageFunc.GetCount(); i++)
			pItem->m_apCreateImageFuncs.Add(g_aCreateImageFuncs[kvrPair.value.anCreateImageFunc[i]]);
		
		pItem->m_strSuffix = kvrPair.value.strSuffix;
		pItem->m_strDescription = kvrPair.value.strDescription;
		
		if (kvrPair.value.anCommands.GetCount() == 1)
		{
			rif_image_filter pFilter = nullptr;
			int nRet = RIF_FillBaseFilterParameters(g_pDeviceForm->GetDeviceIndex(), g_AppConfig.strKernelsCachePath,
				pItem, kvrPair.value.mapParams, &pFilter);
			if (nRet != RIF_SUCCESS)
			{
				if (pFilter)
					RIF_DeleteFilter(pFilter);
					
				ErrorOK(PLAIN_TEXT(g_strLastBackendError));
			}
		}
		else
		{
			rif_image_filter pFilter = nullptr;
			int nRet = RIF_FillCustomFilterParameters(g_pDeviceForm->GetDeviceIndex(), g_AppConfig.strKernelsCachePath,
				pItem, kvrPair.value.mapParams, kvrPair.value.astrCommandAliases, &pFilter);
			if (nRet != RIF_SUCCESS)
			{
				if (pFilter)
					RIF_DeleteFilter(pFilter);
					
				ErrorOK(PLAIN_TEXT(g_strLastBackendError));
			}
		}
				
		m_mapFilterDesc.Add(kvrPair.key, pItem);
	}
	
	GuiLock* pGuiLock = nullptr;
	if (bGuiLock)
		pGuiLock = new GuiLock();
	
	for(const String& strKey : m_mapFilterDesc.GetKeys())
		m_ctrlFilter.Add(strKey);
	
	m_ctrlFilter.SetIndex(0);
	SelectFilter();
	
	g_AppConfig.mapFilters.Clear();
	
	if (pGuiLock)
		delete pGuiLock;
}

void RIFSimple::DestroyFilterMap()
{
	
	for (const FilterDesc* pItem : m_mapFilterDesc)
	{
		for (const FilterParam* pParamItem : pItem->m_mapParams)
			delete pParamItem;
			
		delete pItem;
	}
}

void RIFSimple::SelectFilter()
{
	m_ctrlParams.Clear();
	FilterDesc* pDesc = m_mapFilterDesc.Get(m_ctrlFilter.Get());
	m_ctrlLblFilterInfo = pDesc->GetCommandCount() == 1 ? pDesc->m_strDescription : g_strCustomPrefix + pDesc->m_strDescription;
	
	for(const KeyValueRef<String, FilterParam*>& kvrPair : ~pDesc->m_mapParams)
	{
		FilterParam* pParam = kvrPair.value;
		m_ctrlParams.Add(kvrPair.key, pParam->GetTypeStr(), pParam->m_strValue, pParam->m_strDescription);
	}
}

void RIFSimple::FillFilterMapParams(FilterDesc* pDesc)
{
	for (int i = 0; i < m_ctrlParams.GetCount(); i++)
	{
		ValueArray line = m_ctrlParams.GetArray(i);
		FilterParam* pParam = pDesc->m_mapParams.Get(line[0]);
		
		pParam->m_strValue = line[2];
	}
}

void RIFSimple::AddDirectory()
{
	m_ctrlFS.Multi(false);
	if (m_ctrlFS.ExecuteSelectDir())
	{
		Vector<String> files = FindAllPaths(~m_ctrlFS, (String)~m_ctrlFilePattern, FINDALLFILES);
		for(String& strItem : files)
			m_ctrlFileList.Add(strItem);
	}
}

void RIFSimple::AddFiles()
{
	m_ctrlFS.Multi(true);
	m_ctrlFS.ClearTypes();
	
	String strFilePattern = ~m_ctrlFilePattern;
	m_ctrlFS.Types(Format(g_strFileSelPattern, strFilePattern, strFilePattern));
	if (m_ctrlFS.ExecuteOpen())
	{
		for (int i = 0; i < m_ctrlFS.GetCount(); i++)
			m_ctrlFileList.Add(m_ctrlFS.GetFile(i));
	}
}

void RIFSimple::RemoveFiles()
{
	if (m_ctrlFileList.IsCursor())
		m_ctrlFileList.RemoveSelection();
	else
		Exclamation(PLAIN_TEXT(g_strSelectFiles));
}

void RIFSimple::RemoveAllFiles()
{
	m_ctrlFileList.Clear();
}

bool RIFSimple::ConvertSelectedInThread(String& strMsg)
{
	FilterDesc* pFilterDesc = m_mapFilterDesc.Get(m_ctrlFilter.Get());
	FillFilterMapParams(pFilterDesc);
		
	int nResult = RIF_CreateObjects(g_pDeviceForm->GetDeviceIndex(), g_AppConfig.strKernelsCachePath, pFilterDesc);
	if (nResult != RIF_SUCCESS)
	{
		strMsg = g_strLastBackendError;
		return false;
	}
	
	bool bIndexed = pFilterDesc->m_pExecuteFunc == FilterExecute_Full;
	String strFilename = m_ctrlFileList.Get(0);
	String strOutFilename = ComposeOutFilename(strFilename, ~m_ctrlOutPattern, pFilterDesc->m_strSuffix, bIndexed);
		
	if (!g_AppConfig.strPerfLogFilename.IsEmpty())
	{
		String strFilenameOnly = GetFileName(strFilename);
		ArrayMap<String, RIF_PerfStatWrapper> aPerfStats = { { strFilenameOnly, RIF_PerfStatWrapper() } };
		
		nResult = pFilterDesc->m_pExecuteFunc(strFilename, strOutFilename, &aPerfStats[0]);
		strMsg = aPerfStats[0].ToQTFSimpleTable(m_ctrlFilter.Get(), strFilenameOnly, GetFileName(strOutFilename));
	}
	else
	{
		nResult = pFilterDesc->m_pExecuteFunc(strFilename, strOutFilename, nullptr);
	}
		
	if (nResult != RIF_SUCCESS)
	{
		strMsg = g_strLastBackendError;
		return false;
	}
	
	if (bIndexed)
		strMsg = Format(strMsg, pFilterDesc->GetCommandCount() - 1);
			
	return true;
}

void RIFSimple::ConvertSelected()
{
	if (!m_ctrlFileList.IsCursor() || m_ctrlFileList.GetSelectCount() > 1)
	{
		Exclamation(PLAIN_TEXT(g_strSelectOneFile));
		return;
	}
	
	m_ctrlStatus = g_strStatusInProgress;
	EnableControls(false);
	m_thrConvertSelected.Run( [=]
	{
		String strMsg;
		bool bResult = ConvertSelectedInThread(strMsg);
		RIF_DeleteFilters();
			
		GuiLock __;
		m_ctrlStatus = g_strStatusReady;
		if (bResult)
		{
			m_tcConvertSelectedCB.KillSet(0, [=]
			{
				PromptOK(strMsg);
				ShowPreview(m_ctrlFileList.Get(0));
				EnableControls(true);
			});
		}
		else
		{
			m_tcConvertSelectedCB.KillSet(0, [=]
			{
				ErrorOK(PLAIN_TEXT(strMsg));
				EnableControls(true);
			});
		}
	});
}

void RIFSimple::ConvertAll()
{
	if (m_ctrlFileList.GetCount() > 0)
	{
		FilterDesc* pDesc = m_mapFilterDesc.Get(m_ctrlFilter.Get());
		FillFilterMapParams(pDesc);
		g_pBatchForm->SetFilterDesc(pDesc);
		
		g_pBatchForm->Title(Format(g_strBatchConvertWindowPattern, m_ctrlFilter.Get()));
		g_pBatchForm->FillFileList(m_ctrlFileList);
		g_pBatchForm->Execute();
	}
	else
	{
		Exclamation(PLAIN_TEXT(g_strFilelistEmpty));
	}
}

void RIFSimple::About()
{
	PromptOK(Format(g_strAboutPrettyPattern, APP_VER, g_strAPI));
}

void RIFSimple::ShowPreview(const String& strFilename1)
{
	FilterDesc* pFilterDesc = m_mapFilterDesc.Get(m_ctrlFilter.Get());
	
	bool bIndexed = pFilterDesc->m_pExecuteFunc == FilterExecute_Full;
	String strOutPattern = g_pMainForm->GetOutPattern();
	String strFilename2 = ComposeOutFilename(strFilename1, strOutPattern, pFilterDesc->m_strSuffix, bIndexed);
	
	if (bIndexed)
		strFilename2 = Format(strFilename2, pFilterDesc->GetCommandCount() - 1);
	
	if (!FileExists(strFilename1))
	{
		ErrorOK(PLAIN_TEXT(Format(g_strFileNotFoundPattern, strFilename1)));
		return;
	}
	
	if (!FileExists(strFilename2))
	{
		if (g_pViewerForm->LoadImageByIndex(0, strFilename1, LI_FLAG_SHOW_VIEWER))
		{
			g_pViewerForm->UpdateTitle();
			g_pViewerForm->Execute();
		}
		else
		{
			Exclamation(PLAIN_TEXT(g_strLastBackendError));
		}
		
		return;
	}
	
	if (g_pViewerForm->LoadImageByIndex(0, strFilename1, LI_FLAG_SHOW_VIEWER) &&
		g_pViewerForm->LoadImageByIndex(1, strFilename2, LI_FLAG_SHOW_VIEWER | LI_FLAG_SET_HALF_CW))
	{
		g_pViewerForm->UpdateTitle();
		g_pViewerForm->Execute();
	}
	else
	{
		g_pViewerForm->ClearImageByIndex(0);
		g_pViewerForm->ClearImageByIndex(1);
		Exclamation(PLAIN_TEXT(g_strLastBackendError));
	}
}

void RIFSimple::ShowPreviewDblClick()
{
	if (m_ctrlFileList.IsCursor())
		ShowPreview(m_ctrlFileList.Get(0));
}

void RIFSimple::FileMenu(Bar& ctrlBar)
{
	ctrlBar.Add(g_strMenuAddFiles, [=] { AddFiles(); });
	ctrlBar.Add(g_strMenuAddDir, [=] { AddDirectory(); });
	ctrlBar.Add(g_strMenuRemoveFiles, [=] { RemoveFiles(); });
	ctrlBar.Add(g_strMenuRemoveAll, [=] { RemoveAllFiles(); });
	ctrlBar.Separator();
	ctrlBar.Add(g_strMenuExit, [=] { Close(); });
}

void RIFSimple::DeviceMenu(Bar& ctrlBar)
{
	ctrlBar.Add(g_strSelectDevice, [=]
	{
		g_pDeviceForm->Title(g_strSelectDevice);
		g_pDeviceForm->ShowControls(BIS_DONE);
		g_pDeviceForm->Execute();
	});
}

void RIFSimple::HelpMenu(Bar& ctrlBar)
{
	ctrlBar.Add(g_strMenuAbout, [=] { About(); });
}

void RIFSimple::MainMenu(Bar& ctrlBar)
{
	m_ctrlMenu.Sub(g_strMenuFile, [=] (Bar& b) { FileMenu(b); });
	m_ctrlMenu.Sub(g_strMenuDevice, [=] (Bar& b) { DeviceMenu(b); });
	m_ctrlMenu.Sub(g_strMenuHelp, [=] (Bar& b) { HelpMenu(b); });
}

RIFSimpleViewer::RIFSimpleViewer()
{
	g_pViewerForm = this;
	
	CtrlLayout(*this);
	Sizeable(true).Zoomable(true);
		
	AddFrame(m_ctrlMenu);
	AddFrame(m_ctrlStatus);
	
	Add(m_ctrlBackground.Color(Null).HSizePos(5, 5).VSizePos(5, 5));
	Add(m_actrlViewers[0].HSizePos(5, 5).VSizePos(5, 5));
	Add(m_actrlViewers[1].HSizePos(5, 5).VSizePos(5, 5));
	
	m_actrlViewers[1].SetUseCW(true);
	m_actrlViewers[1].Hide();
	
	m_ctrlFS.Multi(false);
	m_ctrlFS.Types(g_strFileSelAll);
	m_ctrlFS.ActiveDir(GetExeFolder());
	
	m_ctrlColorPick.WhenSelect = [=]
	{
		if (m_eColorPickMode == CPM_BACKGROUND)
			m_ctrlBackground.Color(m_ctrlColorPick.Get());
		else if (m_eColorPickMode == CPM_SEPARATOR)
			m_actrlViewers[1].SetSeparatorColor(m_ctrlColorPick.Get());
	};
	
	m_ctrlMenu.Set( [=] (Bar& b) { MainMenu(b); } );
	
	WhenClose = [=]
	{
		m_aImages[0].Clear();
		m_aImages[1].Clear();
		m_actrlViewers[0].SetImage(Null);
		m_actrlViewers[1].SetImage(Null);
		
		Close();
	};
	
	m_bEnlarge = false;
	m_asizePrev[0].Clear();
	m_asizePrev[1].Clear();
}

void RIFSimpleViewer::SelectBackgroundColor()
{
	m_eColorPickMode = CPM_BACKGROUND;
	m_ctrlColorPick.PopUp(&m_ctrlMenu, m_ctrlBackground.GetBackground());
}

void RIFSimpleViewer::SelectSeparatorColor()
{
	m_eColorPickMode = CPM_SEPARATOR;
	m_ctrlColorPick.PopUp(&m_ctrlMenu, m_actrlViewers[1].GetSeparatorColor());
}

void RIFSimpleViewer::ShowHideSeparator()
{
	m_actrlViewers[1].SetShowSeparator(!m_actrlViewers[1].GetShowSeparator());
}

void RIFSimpleViewer::ToggleEnlarge()
{
	m_bEnlarge = !m_bEnlarge;
	
	m_bForceRedraw[0] = true;
	m_bForceRedraw[1] = true;
	Layout();
}

Image RIFSimpleViewer::CreateSolidImage(Color clrFill, int nWidth, int nHeight)
{
	if (IsNull(clrFill))
		return Null;
	
	ImageBuffer buf(nWidth, nHeight);
	buf.SetKind(IMAGE_OPAQUE);
	
	RGBA pixel;
	pixel.a = 255;
	pixel.r = clrFill.GetR();
	pixel.g = clrFill.GetG();
	pixel.b = clrFill.GetB();
	
	memset32(buf.Begin(), *(dword*)&pixel, nWidth * nHeight);
	
	return buf;
}

void RIFSimpleViewer::ViewMenu(Bar& ctrlBar)
{
	Image imgBGColor = CreateSolidImage(m_ctrlBackground.GetBackground(), SOLID_IMAGE_DIMS, SOLID_IMAGE_DIMS);
	Image imgSepColor = CreateSolidImage(m_actrlViewers[1].GetSeparatorColor(), SOLID_IMAGE_DIMS, SOLID_IMAGE_DIMS);
	
	ctrlBar.Add(g_strMenuEnlarge, [=] { ToggleEnlarge(); }).Check(m_bEnlarge);
	ctrlBar.Separator();
	ctrlBar.Add(g_strMenuBGColor, [=] { SelectBackgroundColor(); }).Image(imgBGColor);
	ctrlBar.Add(g_strMenuSepColor, [=] { SelectSeparatorColor(); }).Image(imgSepColor);
	ctrlBar.Separator();
	ctrlBar.Add(g_strMenuSepShow, [=] { ShowHideSeparator(); }).Check(m_actrlViewers[1].GetShowSeparator());
}

void RIFSimpleViewer::FileMenu(Bar& ctrlBar)
{
	ctrlBar.Add(g_strMenuOpenFirst, [=]
	{
		CompareOpen(0, LI_FLAG_SHOW_ALERT | LI_FLAG_REFRESH_LAYOUT | LI_FLAG_SHOW_VIEWER);
	});
	
	ctrlBar.Add(g_strMenuOpenSecond, [=]
	{
		CompareOpen(1, LI_FLAG_SHOW_ALERT | LI_FLAG_REFRESH_LAYOUT | LI_FLAG_SHOW_VIEWER | LI_FLAG_SET_HALF_CW);
	});
	
	ctrlBar.Add(g_strMenuSwap, [=] { CompareSwap(); }).Enable(IsNull(!m_aImages[0]) || !IsNull(m_aImages[1]));
	ctrlBar.Add(g_strMenuClearFirst, [=] { CompareClear(0); }).Enable(!IsNull(m_aImages[0]));
	ctrlBar.Add(g_strMenuClearSecond, [=] { CompareClear(1); }).Enable(!IsNull(m_aImages[1]));
	ctrlBar.Separator();
	ctrlBar.Add(g_strMenuExit, [=] { Close(); });
}

void RIFSimpleViewer::MainMenu(Bar& ctrlBar)
{
	m_ctrlMenu.Sub(g_strMenuFile, [=] (Bar& b) { FileMenu(b); });
	m_ctrlMenu.Sub(g_strMenuView, [=] (Bar& b) { ViewMenu(b); });
}

void RIFSimpleViewer::CompareOpen(int nIndex, unsigned int dwFlags)
{
	if (m_ctrlFS.ExecuteOpen())
	{
		LoadImageByIndex(nIndex, m_ctrlFS.Get(), dwFlags);
		m_astrFilenames[nIndex] = m_ctrlFS.Get();
		
		UpdateTitle();
		UpdateStatus();
	}
}

bool RIFSimpleViewer::LoadImageByIndex(int nIndex, const String& strFilename, unsigned int dwFlags)
{
	ImageBuffer* pBuffer = nullptr;
	int nRet = RIF_LoadImageIntoBuffer(strFilename, &pBuffer, &m_aDescs[nIndex]);
			
	if (nRet == RIF_SUCCESS)
	{
		m_aImages[nIndex] = *pBuffer;
		delete pBuffer;
		
		m_astrFilenames[nIndex] = strFilename;
		m_bForceRedraw[nIndex] = true;
		
		if (dwFlags & LI_FLAG_SHOW_VIEWER)
			m_actrlViewers[nIndex].Show();
		
		if (dwFlags & LI_FLAG_SET_HALF_CW)
			m_actrlViewers[nIndex].SetCWPercent(0.5f);
		
		if (dwFlags & LI_FLAG_REFRESH_LAYOUT)
			Layout();
		
		return true;
	}
	else if (dwFlags & LI_FLAG_SHOW_ALERT)
	{
		Exclamation(PLAIN_TEXT(g_strLastBackendError));
	}
	
	m_astrFilenames[nIndex].Clear();
	return false;
}

void RIFSimpleViewer::CompareSwap()
{
	Image imgBackup = m_aImages[1];
	m_aImages[1] = m_aImages[0];
	m_aImages[0] = imgBackup;
	
	rif_image_desc descBackup = m_aDescs[1];
	m_aDescs[1] = m_aDescs[0];
	m_aDescs[0] = descBackup;
	
	Size sizeBackup = m_asizePrev[1];
	m_asizePrev[1] = m_asizePrev[0];
	m_asizePrev[0] = sizeBackup;
	
	String strBackup = m_astrFilenames[1];
	m_astrFilenames[1] = m_astrFilenames[0];
	m_astrFilenames[0] = strBackup;
	
	m_actrlViewers[0].SetImage(m_aImages[0]);
	m_actrlViewers[1].SetImage(m_aImages[1]);
	
	m_actrlViewers[1].SetCWPercent(0.5f);
	
	UpdateTitle();
	
	m_bForceRedraw[0] = true;
	m_bForceRedraw[1] = true;
	Layout();
}

void RIFSimpleViewer::CompareClear(int nIndex)
{
	m_aImages[nIndex].Clear();
	m_actrlViewers[nIndex].SetImage(Null);
	m_actrlViewers[nIndex].Hide();
	
	UpdateTitle();
	UpdateStatus();
}

bool RIFSimpleViewer::RescaleImageByIndex(int nIndex, bool bUseMainSize)
{
	Size sizeViewer = m_actrlViewers[nIndex].GetSize();
	Size sizeImage = m_aImages[nIndex].GetSize();
	
	if (!bUseMainSize)
	{
		if (m_bEnlarge || sizeViewer.cx < sizeImage.cx)
		{
			float fRatio = (float)sizeImage.cx / (float)sizeViewer.cx;
			sizeImage.cy = int((float) sizeImage.cy / fRatio);
			sizeImage.cx = sizeViewer.cx;
		}
			
		if (sizeViewer.cy < sizeImage.cy)
		{
			float fRatio = (float)sizeImage.cy / (float)sizeViewer.cy;
			sizeImage.cx = int((float) sizeImage.cx / fRatio);
			sizeImage.cy = sizeViewer.cy;
		}
	}
	else
	{
		sizeImage = m_asizePrev[0];
	}
		
	Size sizeDiff = sizeImage - m_asizePrev[nIndex];
	if (abs(sizeDiff.cx) > 1 || abs(sizeDiff.cy) > 1 || m_bForceRedraw[nIndex])
	{
		m_bForceRedraw[nIndex] = false;
			
		Image imgRescaled = Rescale(m_aImages[nIndex], sizeImage);
		m_actrlViewers[nIndex].SetImage(imgRescaled);
		m_asizePrev[nIndex] = sizeImage;
			
		int nPerc = int((float)sizeImage.cx / (float)m_aImages[nIndex].GetSize().cx * 100.0f);
		m_astrStatuses[nIndex] = Format(g_strPreviewStatusPattern, (int)m_aDescs[nIndex].image_width, (int)m_aDescs[nIndex].image_height,
			(int)m_aDescs[nIndex].num_components, g_astrComponentTypeStr[m_aDescs[nIndex].type - 1], nPerc);
			
		return true;
	}
	
	return false;
}

void RIFSimpleViewer::UpdateTitle()
{
	if (!IsNull(m_aImages[0]) && !IsNull(m_aImages[1]))
		Title(Format(g_strComparePairPattern, m_astrFilenames[0], m_astrFilenames[1]));
	else if (!IsNull(m_aImages[0]))
		Title(Format(g_strComparePairPattern, m_astrFilenames[0], g_strCompareNothing));
	else if (!IsNull(m_aImages[1]))
		Title(Format(g_strComparePairPattern, g_strCompareNothing, m_astrFilenames[1]));
	else
		Title(g_strCompareNothingBoth);
}

void RIFSimpleViewer::UpdateStatus()
{
	if (!IsNull(m_aImages[0]) && !IsNull(m_aImages[1]))
		m_ctrlStatus = Format(g_strComparePairPattern, m_astrStatuses[0], m_astrStatuses[1]);
	else if (!IsNull(m_aImages[0]))
		m_ctrlStatus = Format(g_strComparePairPattern, m_astrStatuses[0], g_strCompareNothing);
	else if (!IsNull(m_aImages[1]))
		m_ctrlStatus = Format(g_strComparePairPattern, g_strCompareNothing, m_astrStatuses[1]);
	else
		m_ctrlStatus = g_strCompareNothingBoth;
}

void RIFSimpleViewer::Layout()
{
	bool bNeedsUpdate = false;
	
	if (!IsNull(m_aImages[0]))
		bNeedsUpdate |= RescaleImageByIndex(0, false);
	
	if (!IsNull(m_aImages[1]))
		bNeedsUpdate |= RescaleImageByIndex(1, true);
	
	if (bNeedsUpdate)
		UpdateStatus();
	
	TopWindow::Layout();
}

RIFSimpleDeviceSel::RIFSimpleDeviceSel()
{
	g_pDeviceForm = this;
	
	CtrlLayout(*this, g_strSelectDevice);
	m_ctrlOK << [=]
	{
		if (!g_pMainForm->IsFilterMapInitted())
			ShowControls(BIS_FILTER_PARAMS);
		else
			Close();
	};
	
	WhenClose = [=]
	{
		if (m_eBackendResult != GR_NA)
		{
			if (!g_pMainForm->IsFilterMapInitted())
				ShowControls(BIS_FILTER_PARAMS);
			else
				Close();
		}
		else
			Exclamation(PLAIN_TEXT(g_strBackgroundTaskInProgress));
	};
		
	m_eBackendResult = GR_NA;
}

void RIFSimpleDeviceSel::InitBackend()
{
	int nRet = RIF_AttachLog(g_AppConfig.sLogging.nLevel, g_AppConfig.sLogging.strFilename);
	if (nRet != RIF_SUCCESS)
	{
		m_eBackendResult = GR_FAIL;
		m_strBackendMsg = g_strLastBackendError;
		return;
	}
	
	int nDeviceCount = 0;
	nRet = RIF_GetDeviceCount(&nDeviceCount);
	
	if (nRet != RIF_SUCCESS)
	{
		m_eBackendResult = GR_FAIL;
		m_strBackendMsg = g_strLastBackendError;
		return;
	}
	
	bool bSoftFail = false;
	bool bCompleteFail = true;
		
	for (int i = 0; i < nDeviceCount; i++)
	{
		String strName;
		nRet = RIF_GetDeviceName(i, strName);
			
		if (nRet != RIF_SUCCESS)
		{
			bSoftFail = true;
			bCompleteFail &= true;
			m_astrDevices.Add(g_strLastBackendError);
		}
		else
		{
			bCompleteFail &= false;
			m_astrDevices.Add(strName);
		}
	}
		
	if (!bCompleteFail)
	{
		if (!bSoftFail)
		{
			m_eBackendResult = GR_SUCCESS;
			m_strBackendMsg = g_strDoneNoErrors;
		}
		else
		{
			m_eBackendResult = GR_SOFTFAIL;
			m_strBackendMsg = g_strDoneWithErrors;
		}
	}
	else
	{
		m_eBackendResult = GR_FAIL;
		m_strBackendMsg = g_strGetDeviceInfoFail;
	}
	
}

bool RIFSimpleDeviceSel::DoneWithBackend()
{
	GuiLock __;
	m_ctrlLblText = m_strBackendMsg;
		
	if (m_eBackendResult != GR_FAIL)
	{
		if (m_eBackendResult == GR_SUCCESS)
			m_ctrlLblText.SetInk(Green());
		else
			m_ctrlLblText.SetInk(LtRed());
		
		for (const String& strItem : m_astrDevices)
			m_ctrlDevices.Add(strItem);
		
		m_ctrlDevices.SetIndex(0);
		return true;
	}
	else
	{
		m_ctrlLblText.SetInk(Red());
		return false;
	}

}

void RIFSimpleDeviceSel::ShowControls(eBackendInitState eState)
{
	switch (eState)
	{
		case BIS_BACKEND:
		{
			m_ctrlDevices.Hide();
			m_ctrlOK.Hide();
			m_ctrlLblText.Show();
			
			m_ctrlLblText.SetInk(Blue());
			m_ctrlLblText = g_strInitializingBackend;
			
			m_thrInitBackend.Run( [=]
			{
				InitBackend();
				if (DoneWithBackend())
				{
					if (!g_AppConfig.bQuickStart || m_eBackendResult != GR_SUCCESS)
						Thread::Sleep(DONE_WITH_BACKEND_DELAY);
					
					ShowControls(BIS_DONE);
				}
			});
		}
		break;
		
		case BIS_FILTER_PARAMS:
		{
			m_ctrlDevices.Hide();
			m_ctrlOK.Hide();
			m_ctrlLblText.Show();
			
			m_ctrlLblText.SetInk(Blue());
			m_ctrlLblText = g_strQueryingFilterParams;
			
			m_eBackendResult = GR_NA;
			
			m_thrQueryFilterParams.Run( [=]
			{
				g_pMainForm->InitFilterMap(true);
				m_eBackendResult = GR_SUCCESS;
				ShowControls(BIS_DONE_AND_CLOSE);
			});
		}
		break;
		
		case BIS_DONE:
		case BIS_DONE_AND_CLOSE:
		default:
		{
			m_ctrlLblText.Hide();
			m_ctrlDevices.Show();
			m_ctrlOK.Show();
			
			if (eState == BIS_DONE_AND_CLOSE)
				Close();
		};
		break;
	}
}

void RIFSimpleBatchConv::Reset()
{
	m_ctrlProgress.Set(0, 1);
	m_ctrlStart.Enable(true);
	m_ctrlStop.Enable(false);
	m_ctrlLog.Clear();
}

void RIFSimpleBatchConv::FillFileList(ArrayCtrl& ctrlSource)
{
	int nFileCount = ctrlSource.GetCount();
	
	m_ctrlProgress.Set(0, nFileCount);
	m_ctrlStart.Enable(true);
	m_ctrlStop.Enable(false);
	
	for (int i = 0; i < nFileCount; i++)
		m_ctrlLog.Add(ctrlSource.Get(i, 0), g_strFileStatusInQueue);
}

void RIFSimpleBatchConv::SetItemStatusInThread(int nIndex, const String& strStatus, const String& strMsg, bool bUpdateProgress)
{
	GuiLock __;
	m_ctrlLog.Set(nIndex, 1, !IsNull(strMsg) ? strStatus + strMsg : strStatus);
	
	if (bUpdateProgress)
		m_ctrlProgress++;
}

bool RIFSimpleBatchConv::ConvertInThread(int nIndex, String& strMsg, ArrayMap<String, RIF_PerfStatWrapper>& aPerfStats)
{
	int nResult = RIF_CreateObjects(g_pDeviceForm->GetDeviceIndex(), g_AppConfig.strKernelsCachePath, m_pFilterDesc);
	if (nResult != RIF_SUCCESS)
	{
		strMsg = g_strLastBackendError;
		return false;
	}

	String strFilename = m_ctrlLog.Get(nIndex, 0);
	String strOutFilename = ComposeOutFilename(strFilename, g_pMainForm->GetOutPattern(), m_pFilterDesc->m_strSuffix,
		m_pFilterDesc->m_pExecuteFunc == FilterExecute_Full);
	
	if (!g_AppConfig.strPerfLogFilename.IsEmpty())
	{
		aPerfStats.Add(GetFileName(strFilename), RIF_PerfStatWrapper());
		nResult = m_pFilterDesc->m_pExecuteFunc(strFilename, strOutFilename, &aPerfStats[nIndex]);
	}
	else
		nResult = m_pFilterDesc->m_pExecuteFunc(strFilename, strOutFilename, nullptr);
	
	if (nResult != RIF_SUCCESS)
	{
		strMsg = g_strLastBackendError;
		return false;
	}
		
	return true;
}

void RIFSimpleBatchConv::ConvertStart()
{
	m_ctrlStart.Disable();
	m_ctrlStop.Enable();
	m_eConvertResult = GR_PROGRESS;
	
	int nFileCount = m_ctrlLog.GetCount();
	m_ctrlProgress.Set(0, nFileCount);
	
	for (int i = 0; i < nFileCount; i++)
		m_ctrlLog.Set(i, 1, g_strFileStatusInQueue);
	
	m_thrConvertAll.Run( [=]
	{
		ArrayMap<String, RIF_PerfStatWrapper> aPerfStats;
		for (int i = 0; i < nFileCount; i++)
		{
			if (m_eConvertResult == GR_CANCELLED)
				break;
			
			String strMsg = Null;
			SetItemStatusInThread(i, g_strFileStatusInProgress, strMsg, false);
					
			bool bResult = ConvertInThread(i, strMsg, aPerfStats);
			
			if (bResult)
				SetItemStatusInThread(i, g_strFileStatusDone, strMsg, true);
			else
				SetItemStatusInThread(i, g_strFileStatusFailed, strMsg, true);
		}
		
		if (!g_AppConfig.strPerfLogFilename.IsEmpty())
			StoreAsJsonFile(aPerfStats, GetFileDirectory(~m_ctrlLog.Get(0, 0)) +
				Format(g_AppConfig.strPerfLogFilename, m_pFilterDesc->m_strSuffix), true);

			
		ConvertEnd();
	});
}

void RIFSimpleBatchConv::ConvertStop()
{
	m_ctrlStop.Disable();
	m_eConvertResult = GR_CANCELLED;
}

void RIFSimpleBatchConv::ConvertEnd()
{
	RIF_DeleteFilters();
	
	GuiLock __;
	m_eConvertResult = GR_SUCCESS;
	m_ctrlStart.Enable();
	m_ctrlStop.Disable();
}

void RIFSimpleBatchConv::ShowPreview()
{
	if (!m_ctrlLog.IsCursor())
		return;
	
	bool bIndexed = m_pFilterDesc->m_pExecuteFunc == FilterExecute_Full;
	String strOutPattern = g_pMainForm->GetOutPattern();
	String strFilename1 = m_ctrlLog.Get(0);
	String strFilename2 = ComposeOutFilename(strFilename1, strOutPattern, m_pFilterDesc->m_strSuffix, bIndexed);
	
	if (bIndexed)
		strFilename2 = Format(strFilename2, m_pFilterDesc->GetCommandCount() - 1);
	
	if (!FileExists(strFilename1))
	{
		ErrorOK(PLAIN_TEXT(Format(g_strFileNotFoundPattern, strFilename1)));
		return;
	}
	
	if (!FileExists(strFilename2))
	{
		if (g_pViewerForm->LoadImageByIndex(0, strFilename1, LI_FLAG_SHOW_VIEWER))
		{
			g_pViewerForm->UpdateTitle();
			g_pViewerForm->Execute();
		}
		else
		{
			Exclamation(PLAIN_TEXT(g_strLastBackendError));
		}
		
		return;
	}
	
	if (g_pViewerForm->LoadImageByIndex(0, strFilename1, LI_FLAG_SHOW_VIEWER) &&
		g_pViewerForm->LoadImageByIndex(1, strFilename2, LI_FLAG_SHOW_VIEWER | LI_FLAG_SET_HALF_CW))
	{
		g_pViewerForm->UpdateTitle();
		g_pViewerForm->Execute();
	}
	else
	{
		g_pViewerForm->ClearImageByIndex(0);
		g_pViewerForm->ClearImageByIndex(1);
		Exclamation(PLAIN_TEXT(g_strLastBackendError));
	}
}

RIFSimpleBatchConv::RIFSimpleBatchConv()
{
	g_pBatchForm = this;
	
	CtrlLayout(*this);
	Zoomable(true).Sizeable(true);
	m_ctrlProgress.Percent();
	
	m_ctrlStart << [=] { ConvertStart(); };
	m_ctrlStop << [=] { ConvertStop(); };
	
	WhenClose = [=]
	{
		if (m_eConvertResult != GR_PROGRESS)
		{
			Reset();
			Close();
		}
		else
		{
			Exclamation(PLAIN_TEXT(g_strConversionInProgress));
		}
	};
	
	m_ctrlLog.WhenLeftDouble = [=] { ShowPreview(); };
	m_ctrlLog.AddColumn(g_strColFilename);
	m_ctrlLog.AddColumn(g_strColStatus).SetDisplay(m_dispFileAndStatus);
	
	Reset();
}

GUI_APP_MAIN
{
	GUI_DragDistance_Write(1);
	
	LoadFromJsonFile(g_AppConfig, g_strConfigFilename);
	SetLanguage(LNGC_('E','N','U','S', CHARSET_UNICODE));
	
	RIFSimple ctrlMain;
	RIFSimpleViewer ctrlViewer;
	RIFSimpleDeviceSel ctrlDeviceSel;
	RIFSimpleBatchConv ctrlBatchConv;
		
	ctrlMain.Run();
}
