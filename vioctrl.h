#ifndef _vioctrl_h
#define _vioctrl_h

#include <CtrlLib/CtrlLib.h>

using namespace Upp;

//#include "maisutils.h"
#include <VIO/vio.h>


//===============================================================================

struct VIOCtrl : public Ctrl
{
	typedef VIOCtrl CLASSNAME;

	VIO vio;
	Voice sound, stemp;

private:

	struct VolumeSlider : public Ctrl
	{
		typedef VolumeSlider CLASSNAME;
		Callback1<double> WhenSlide;
	private:
		struct Thumb : public Ctrl
		{
			typedef Thumb CLASSNAME;
			VolumeSlider *pvs;
			bool ld;
			int mx;
			int thumbpos;
			Callback1<double> WhenSet;
			Thumb(VolumeSlider *par) { pvs=par; SetRect(0,1,10,18); AddFrame(ButtonFrame()); ld=false; }
			virtual ~Thumb(){}
			void Paint(Draw &drw) { drw.DrawRect(GetSize(), Gray()); }
			void SetThumbPos(int pos, bool bSlide=true)
			{
				Size sz=pvs->GetSize();
				int x=(IsEnabled())?pos:0;
				if (x<0) x=0;
				if (x>(sz.cx-10)) x=(sz.cx-10);
				thumbpos=x;
				SetRectX(x, 10);
				if (WhenSet)
				{
					Size sz=pvs->GetSize();
					double d=((double(thumbpos)/double(sz.cx-10))*1.5); //150%
					if (bSlide) WhenSet(d);
				}
			}
			virtual void LeftDown(Point p, dword kf) { if (IsEnabled()) { ld=true; mx=p.x; SetCapture(); }}
			virtual void LeftUp(Point p, dword kf) { if (IsEnabled()) { ld=false; ReleaseCapture(); }}
			virtual void MouseMove(Point p, dword kf) { if (IsEnabled()) { if (ld) SetThumbPos(thumbpos+p.x-mx); }}
		};
		Thumb thumb;
		void OnSet(double d) { if (WhenSlide) WhenSlide((d*1.5)); } //150%
	public:
		VolumeSlider();
		virtual ~VolumeSlider(){}
		void Paint(Draw &drw);
		void SetVolume(int perc, bool bSlide=true) { Size sz=GetSize(); int p=(int)(double(sz.cx)*(double(perc)/150.0)); thumb.SetThumbPos(p, bSlide); } //150%
		int GetVolume() { Size sz=GetSize(); int p=(int)(150.0*(double(thumb.thumbpos)/double(sz.cx))); return p; } //150%
	};

	struct PicButton : public Button
	{
		Image pic, picX;
		bool ld;
		Callback WhenClick;
		PicButton() : ld(false) {}
		virtual ~PicButton() {}
		PicButton& SetPics(const Image &p, const Image &x) { pic=p; picX=x; return *this; }
		void Paint(Draw &drw)
		{
			Button::Paint(drw);
			Size sz=GetSize();
			if (IsEnabled()) drw.DrawImage((sz.cx - pic.GetWidth())/2, (sz.cy - pic.GetHeight())/2, pic);
			else drw.DrawImage((sz.cx - picX.GetWidth())/2, (sz.cy - picX.GetHeight())/2, picX);
		}
		virtual void LeftDown(Point p, dword keyflags) { ld=true; }
		virtual void LeftUp(Point p, dword keyflags) { if (ld) if (WhenClick) WhenClick(); ld=false; }
	};

	struct VIOTools : public Ctrl
	{
		typedef VIOTools CLASSNAME;
		VIOCtrl &vioc;
		PicButton pbRec, pbPlay, pbPause, pbStop;
		VolumeSlider volume;
		PicButton pbRestore, pbNew, pbExtract;
		
		VIOTools(VIOCtrl &par);
		virtual ~VIOTools() {}

		virtual void MouseEnter(Point p, dword keyflags) { OverrideCursor(Image::Arrow()); }
		
		Callback WhenRecord;
		Callback WhenPlay;
		Callback WhenPause;
		Callback WhenStop;
		Callback1<double> WhenVolume;
		Callback WhenRestore;
		Callback WhenNew;
		Callback WhenExtract;

		void SetStatus();
		
		void OnRec()			{ if (WhenRecord) WhenRecord(); }
		void OnPlay()			{ if (WhenPlay) WhenPlay(); }
		void OnPause()			{ if (WhenPause) WhenPause(); }
		void OnStop()			{ if (WhenStop) WhenStop(); }
		void OnSlide(double d)	{ if (WhenVolume) WhenVolume(d); }
		void OnRestore()		{ if (WhenRestore) WhenRestore(); }
		void OnNew()			{ if (WhenNew) WhenNew(); }
		void OnExtract()		{ if (WhenExtract) WhenExtract(); }
		
	};
	
	struct VIOGraph : public Ctrl
	{
		typedef VIOGraph CLASSNAME;
		
		VIOCtrl &vioc;
		HScrollBar HS;
		int ld, rd;
		bool pl; //set in LeftDown, used in MouseMove
		Point mpt;
		Image prevmouseimg;
		bool bld, bsel, bGrid, b0Line, bScales;
		Voice data;
	
		Callback1<MenuBar&> WhenRClick;
		Callback WhenSelection; //to set tools-status
		Callback WhenRecorded;
		Callback WhenVoice;
	
		VIOGraph(VIOCtrl &par);
		virtual ~VIOGraph(){};
	
		void draw_grid(Draw &drw, int h, int VMax, Color D, Color DD);
		void draw_scales(Draw &drw, int h, int VMax, Color C);
		void draw_graph(Draw &drw, int h, int H, Color CN, Color CS);
		virtual void Paint(Draw &drw);
	
		void Clear() { data.clear(); HS.Set(0); ld=rd=0; bld=bsel=false; Refresh(); }
	
		//void DoHScroll() { Refresh(); }
	
		virtual void RightDown(Point p, dword keyflags);
		virtual void LeftDown(Point p, dword keyflags);
		virtual void LeftUp(Point p, dword keyflags);
		virtual void MouseMove(Point p, dword keyflags);
		virtual void MouseEnter(Point p, dword keyflags);
		virtual void MouseLeave();
	
		void ShowGrid(bool b=true) { bGrid=b; Refresh(); }
		void ShowScales(bool b=true) { bScales=b; Refresh(); }
		void ShowZeroLine(bool b=true) { b0Line=b; Refresh(); }
	
		void Plot(const Voice &v, bool bRedraw=true); // { data=v; if (bRedraw) { ld=rd=0; bsel=false; } HS.SetTotal(v.size()); HS.Set(0); Refresh(); }
	
		bool HasSelection() { return ((rd-ld)>=5); }
		bool GetSelection(int &L, int &R) { L=ld; R=rd; return ((R-L)>=5); }
		bool GetSelection(Voice &v);
		
		void SetSelection(int L, int R);
		
	};
	
	VIOTools tools;
	VIOGraph graph;
	
	bool b_is_recording;
	
	struct VoiceStats
	{
		size_t	nMaxima, nMinima, nInterval;
		double	damphigh, damplow, daverage, dmean, dfreq, dwavelen;
	};
	
	bool GetVoiceStats(const Voice &us, VoiceStats &ss);

	void on_selection();
	
public:

	Callback1<MenuBar&> WhenBar;

	void volume_setup();
	
	VIOCtrl();
	virtual ~VIOCtrl() {}
	
	void Layout();
	virtual void MouseEnter(Point p, dword keyflags) { OverrideCursor(Image::Arrow()); }
	
	void ClearVIO() { sound.clear(); ResetVIO(); }
	void ResetVIO(bool bAll=true);
	void SetVoice(const Voice &v);
	void OnMenuBar(MenuBar &bar) { if (WhenBar) WhenBar(bar); }
	void Volume(double dvol); //current sound

	void Record();
	void Play();
	void Pause();
	void Stop();
	void Restore();
	void New();
	void Extract();

	void ExtractClip(Voice &v);

	Callback1<const Voice&> WhenRecorded;
	Callback1<const Voice&> WhenVoice;
	Callback WhenNew;
	
};





#endif
