/*
 * Copyright (c) 2003-2006 Milan Cutka
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "Cwinamp2.h"
#include "winamp2/Twinamp2.h"
#include "winamp2/DSP.H"

void Twinamp2page::init(void)
{
 static const TanchorInfo ainfo[]=
  {
   IDC_GRP_WINAMP2,TanchorInfo::LEFT|TanchorInfo::RIGHT|TanchorInfo::TOP|TanchorInfo::BOTTOM,
   IDC_BT_WINAMP2_DIR,TanchorInfo::LEFT|TanchorInfo::RIGHT|TanchorInfo::TOP,
   IDC_LBX_WINAMP2_DSPS,TanchorInfo::LEFT|TanchorInfo::RIGHT|TanchorInfo::TOP|TanchorInfo::BOTTOM,
   IDC_CBX_WINAMP2_FILTERS,TanchorInfo::LEFT|TanchorInfo::RIGHT|TanchorInfo::BOTTOM,
   IDC_BT_WINAMP2_CONFIG,TanchorInfo::LEFT|TanchorInfo::BOTTOM,
   0,0
  };
 anchors.init(ainfo,*this); 
 winamp2=NULL;
 currentdsp=NULL;
}

void Twinamp2page::cfg2dlg(void)
{
 deciA->getWinamp2(&winamp2);
 dir2dlg();
}

void Twinamp2page::dir2dlg(void)
{
 const char_t *dir=cfgGetStr(IDFF_winamp2dir);
 if (dir[0]!='\0')
  {
   setText(IDC_BT_WINAMP2_DIR,dir);
   dsps2dlg();
  } 
}

void Twinamp2page::dsps2dlg(void)
{
 lbxClear(IDC_LBX_WINAMP2_DSPS);
 const char_t *flnm=cfgGetStr(IDFF_winamp2flnm);
 for (Twinamp2::Twinamp2dspDlls::const_iterator d=winamp2->dsps.begin();d!=winamp2->dsps.end();d++)
  {
   int ii=lbxAdd(IDC_LBX_WINAMP2_DSPS,(*d)->descr.c_str(),intptr_t(*d));
   if ((*d)->descr==flnm)
    lbxSetCurSel(IDC_LBX_WINAMP2_DSPS,ii);
  } 
 filter2dlg();
}

void Twinamp2page::filter2dlg(void)
{
 cbxClear(IDC_CBX_WINAMP2_FILTERS);
 const Twinamp2dspDll *dsp=winamp2->getFilter(cfgGetStr(IDFF_winamp2flnm));
 if (dsp)
  {
   const char_t *filtername=cfgGetStr(IDFF_winamp2filtername);
   for (Twinamp2dspDll::Tfilters::const_iterator f=dsp->filters.begin();f!=dsp->filters.end();f++)
    {
     int ii=cbxAdd(IDC_CBX_WINAMP2_FILTERS,(*f)->descr.c_str(),intptr_t(*f));
     if (strcmp((*f)->descr.c_str(),filtername)==0)
      cbxSetCurSel(IDC_CBX_WINAMP2_FILTERS,ii);
    }
  }
}

void Twinamp2page::selectFilter(const char_t *flnm,const char_t *filtername)
{
 if (currentdsp)
  {
   currentdsp->done();
   currentdsp=NULL;
  }
 if (flnm) cfgSet(IDFF_winamp2flnm,flnm);
 cfgSet(IDFF_winamp2filtername,filtername);
}

INT_PTR Twinamp2page::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch (uMsg)
  {
   case WM_COMMAND:
    switch (LOWORD(wParam))  
     {
      case IDC_LBX_WINAMP2_DSPS:
       if (HIWORD(wParam)==LBN_SELCHANGE)
        {
         const Twinamp2dspDll* f=(const Twinamp2dspDll*)lbxGetCurItemData(IDC_LBX_WINAMP2_DSPS);
         selectFilter(f->descr.c_str(),f->filters[0]->descr.c_str());
         dsps2dlg();
         return TRUE;
        }
       break;
      case IDC_CBX_WINAMP2_FILTERS:
       if (HIWORD(wParam)==CBN_SELCHANGE)
        {
         selectFilter(NULL,cbxGetCurText(IDC_CBX_WINAMP2_FILTERS));
         return TRUE;
        }
       break;
     }
    break;
  }
 return TconfPageDecAudio::msgProc(uMsg,wParam,lParam);
}
void Twinamp2page::onDir(void)
{
 char_t dir[MAX_PATH];cfgGet(IDFF_winamp2dir,dir,MAX_PATH);
 if (dlgGetDir(m_hwnd,dir,_(-IDD_WINAMP2,_l("Select Winamp 2 directory (not the plugins directory!)"))))
  {
   cfgSet(IDFF_winamp2dir,dir);
   deciA->getWinamp2(&winamp2);
   dir2dlg();
   const Twinamp2dspDll* f=(const Twinamp2dspDll*)lbxGetCurItemData(IDC_LBX_WINAMP2_DSPS);
   if (f)
    selectFilter(f->descr.c_str(),f->filters[0]->descr.c_str());
   else
    selectFilter(_l(""),_l("")); 
  }
}
void Twinamp2page::onConfig(void)
{
 if (currentdsp) currentdsp->done();
 currentdsp=(Twinamp2dsp*)cbxGetCurItemData(IDC_CBX_WINAMP2_FILTERS);
 if (currentdsp)
  {
   currentdsp->init();
   currentdsp->config(m_hwnd);
  }
}

Twinamp2page::Twinamp2page(TffdshowPageDec *Iparent,const TfilterIDFF *idff):TconfPageDecAudio(Iparent,idff)
{
 resInter=IDC_CHB_WINAMP2;
 static const TbindButton<Twinamp2page> bt[]=
  {
   IDC_BT_WINAMP2_DIR,&Twinamp2page::onDir,
   IDC_BT_WINAMP2_CONFIG,&Twinamp2page::onConfig,
   0,NULL
  };
 bindButtons(bt); 
}
