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
#include "Twinamp2.h"
#include "Tdll.h"
#include "DSP.H"
#include "Twinamp2settings.h"
#include "ffdebug.h"

//============================== Twinamp2 ====================================
Twinamp2::Twinamp2(const char_t *Iwinampdir)
{
 strcpy(winampdir,Iwinampdir);
 strings files;
 char_t mask[MAX_PATH];_makepath(mask,NULL,winampdir,_l("plugins\\dsp*"),_l("dll"));
 findFiles(mask,files);
 for (strings::const_iterator flnm=files.begin();flnm!=files.end();flnm++)
  {
   Twinamp2dspDll *dsp=new Twinamp2dspDll(*flnm);
   if (!dsp->filters.empty())
    dsps.push_back(dsp);
   else 
    delete dsp;
  }
}
Twinamp2::~Twinamp2()
{
 for (Twinamp2dspDlls::iterator d=dsps.begin();d!=dsps.end();d++)
  (*d)->release();
}

Twinamp2dspDll* Twinamp2::getFilter(const char_t *flnm)
{
 for (Twinamp2dspDlls::iterator d=dsps.begin();d!=dsps.end();d++)
  if ((*d)->descr==flnm)
   return *d;
 return NULL;  
}

Twinamp2dsp* Twinamp2::getFilter(const Twinamp2settings *cfg)
{
 Twinamp2dspDll *d=getFilter(cfg->flnm);
 if (!d) return NULL;
 for (Twinamp2dspDll::Tfilters::iterator f=d->filters.begin();f!=d->filters.end();f++)
  if (strcmp((*f)->descr.c_str(),cfg->modulename)==0)
   {
    d->addref();
    return *f;
   }
 return NULL;
}

//=========================== Twinamp2dspDll =================================
Twinamp2dspDll::Twinamp2dspDll(const ffstring &flnm):refcount(1)
{
 winampDSPGetHeaderType=NULL;
 hdr=NULL;
 dll=new Tdll(flnm.c_str(),NULL);
 dll->loadFunction(winampDSPGetHeaderType,"winampDSPGetHeader2");
 if (dll->ok)
  {
   hdr=winampDSPGetHeaderType();
   if (hdr->version!=DSP_HDRVER) 
    {
     hdr=NULL;
     return;
    }
   descr=hdr->description;
  }
 if (hdr)
  for (int i=0;;i++)
   {
    winampDSPModule *flt=hdr->getModule(i);
    if (!flt) break;
    flt->hDllInstance=dll->hdll;
    filters.push_back(new Twinamp2dsp(this,flt));
   }
}
Twinamp2dspDll::~Twinamp2dspDll()
{
 for (Tfilters::iterator f=filters.begin();f!=filters.end();f++)
  delete *f;
 delete dll;
}
void Twinamp2dspDll::addref(void) 
{
 refcount++;
 DPRINTFA("Twinamp2dspDll: %s: %i",hdr->description,refcount);
}
void Twinamp2dspDll::release(void) 
{
 refcount--;
 DPRINTFA("Twinamp2dspDll: %s: %i",hdr->description,refcount);
 if (refcount==0) 
  {
   DPRINTFA("Twinamp2dspDll: deleting: %s",hdr->description);
   delete this;
  }
}

//============================= Twinamp2dsp ==================================
Twinamp2dsp::Twinamp2dsp(Twinamp2dspDll *Idll,winampDSPModule *Imod):mod(Imod),dll(Idll),h(NULL),hThread(NULL)
{
 descr=mod->description;
 inited=0;
}
int Twinamp2dsp::init(void)
{
 if (!inited++ && mod->Init)
  {
   unsigned threadID;
   hThread=(HANDLE)_beginthreadex(NULL,65536,threadProc,this,NULL,&threadID);
   while (!h) Sleep(20);
   return (h!=(HWND)-1)?1:(h=NULL,0);
  }
 else 
  return 0;
}
void Twinamp2dsp::config(HWND parent)
{
 if (mod->Config)
  {
   mod->hwndParent=parent;
   mod->Config(mod);
  }
}
void Twinamp2dsp::done(void)
{
 if (!inited) return;
 inited--;
 if (inited==0 && h) 
  {
   SendMessage(h,WM_CLOSE,0,0);
   WaitForSingleObject(hThread,INFINITE);
  }
}
void Twinamp2dsp::release(void)
{
 dll->release();
}

size_t Twinamp2dsp::process(int16_t *samples,size_t numsamples,int bps,int nch,int srate)
{
 return mod->ModifySamples?mod->ModifySamples(mod,samples,(int)numsamples,bps,nch,srate):0;
}

LRESULT CALLBACK Twinamp2dsp::wndProc(HWND hwnd, UINT msg, WPARAM wprm, LPARAM lprm)
{
 switch (msg)
  { 
   case WM_DESTROY:
    PostQuitMessage(0);
    break;
  }
 return DefWindowProc(hwnd,msg,wprm,lprm);
}

unsigned int __stdcall Twinamp2dsp::threadProc(void *self0)
{
 Twinamp2dsp *self=(Twinamp2dsp*)self0;
 static const char_t *FFDSHOW_WINAMP_CLASS=_l("ffdshow_winamp_class");
 randomize();
 setThreadName(DWORD(-1),"winamp2");
 
 HINSTANCE hi=self->mod->hDllInstance;
 char_t windowName[80];tsprintf(windowName,_l("%s_window%i"),FFDSHOW_WINAMP_CLASS,rand());
 HWND h=createInvisibleWindow(hi,FFDSHOW_WINAMP_CLASS,windowName,wndProc,self,NULL);
 self->mod->hwndParent=h;
 if (self->mod->Init(self->mod)==0)
  {
   self->h=h;
   if (self->h)
    {
     SetWindowLongPtr(self->h,GWLP_USERDATA,LONG_PTR(self));
     MSG msg;
     while(GetMessage(&msg, NULL, 0, 0))
      {
       TranslateMessage(&msg);
       DispatchMessage(&msg);
      }
    }  
   self->mod->Quit(self->mod);
  }
 else
  {
   self->h=(HWND)-1;
   DestroyWindow(h);
  } 
 UnregisterClass(FFDSHOW_WINAMP_CLASS,hi);
 _endthreadex(0);
 return 0;
}
