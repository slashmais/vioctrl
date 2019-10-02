
#include <utilfuncs/utilfuncs.h>

#include "vioctrl.h"
#include <thread>
#include <chrono>



extern void dodebug(const std::string s);



#define IMAGECLASS VIOPICS
#define IMAGEFILE  <vioctrl/vioctrl.iml>
#include <Draw/iml.h>


//===============================================================================globals

//VIO vio; ????????

VIOCtrl *ptr_vioctrl; //for cb_vio_done() callback
void cb_vio_done(bool bResetAll) { ptr_vioctrl->ResetVIO(bResetAll); } //callback because c++ pointer-to-member bullshit


//===============================================================================VolumeSlider

VIOCtrl::VolumeSlider::VolumeSlider() : thumb(this)
{
	SetRect(0,0,100,20);
	AddFrame(ThinInsetFrame());
	Add(thumb);//.LeftPos(0,10).TopPos(1,16));
	thumb.SetThumbPos(10);
	thumb.WhenSet = THISBACK(OnSet);
	
}
		
void VIOCtrl::VolumeSlider::Paint(Draw &drw)
{
	int i, hd;//, y, yt, yb;
	double h, w;
	Color col;
	
	Size sz=GetSize();
	drw.DrawRect(sz, SColorFace());
	col=(IsEnabled())?Blue():Gray();
	h=(sz.cy-10);
	w=(sz.cx-10);
	if ((sz.cx>12)&&(sz.cy>12))
	{
		for (i=0;i<(int)w;i++)
		{
			hd=(int)((h*(i/w)));
			drw.DrawLine(i+5, sz.cy-5-hd, i+5, sz.cy-5, 1, col);
		}
		drw.DrawLine(5, sz.cy-5, w+5, 5, 1, col);
		drw.DrawLine(5, sz.cy-5, w+5, 6, 1, col);

		drw.DrawLine(w+5, 5, w+5, sz.cy-5, 1, White());
		drw.DrawLine(5, sz.cy-5, w+5, sz.cy-5, 1, White());
	}
}

//===============================================================================tools-ctrl

#define GRAPH_HEIGHT (int)257

VIOCtrl::VIOTools::VIOTools(VIOCtrl &par) : vioc(par)
{
	SetRectY(0, 24);
	AddFrame(ThinInsetFrame());
	
	Add(pbRec.SetPics(VIOPICS::recok(), VIOPICS::recdisable()).LeftPos(1, 22).TopPos(1, 22).Tip("Record"));
	pbRec.WhenClick = THISBACK(OnRec);
	Add(pbPlay.SetPics(VIOPICS::playok(), VIOPICS::playdisable()).LeftPos(25, 22).TopPos(1, 22).Tip("Play"));
	pbPlay.WhenClick = THISBACK(OnPlay);
	Add(pbPause.SetPics(VIOPICS::pauseok(), VIOPICS::pausedisable()).LeftPos(25, 22).TopPos(1, 22).Tip("Pause"));
	pbPause.WhenClick = THISBACK(OnPause);
	Add(pbStop.SetPics(VIOPICS::stopok(), VIOPICS::stopdisable()).LeftPos(49, 22).TopPos(1, 22).Tip("Stop"));
	pbStop.WhenClick = THISBACK(OnStop);
	Add(volume.LeftPos(73,90).TopPos(1,22).Tip("Volume"));
	volume.WhenSlide = THISBACK(OnSlide);
	Add(pbRestore.SetPics(VIOPICS::restore(), VIOPICS::restoredisable()).LeftPos(165, 22).TopPos(1, 22).Tip("Reset"));
	pbRestore.WhenClick = THISBACK(OnRestore);
	Add(pbNew.SetPics(VIOPICS::pnew(), VIOPICS::pnewdisable()).LeftPos(189, 22).TopPos(1, 22).Tip("New"));
	pbNew.WhenClick = THISBACK(OnNew);
	Add(pbExtract.SetPics(VIOPICS::extract(), VIOPICS::extractdisable()).LeftPos(213, 22).TopPos(1, 22).Tip("Copy Clip/Selection"));
	pbExtract.WhenClick = THISBACK(OnExtract);

	SetStatus();
	
}

void VIOCtrl::VIOTools::SetStatus()
{
	pbRec.Enable(vioc.vio.CanRecord());
	if (vioc.vio.IsPlaying()) { pbPlay.Hide(); pbPause.Show(); pbPause.Enable(); }
	else { pbPause.Hide(); pbPlay.Show(); pbPlay.Enable(vioc.vio.CanPlay()&&!vioc.sound.empty()); }
	pbStop.Enable(vioc.vio.IsRecording()||vioc.vio.IsPlaying()||vioc.vio.IsPaused());
	volume.Enable(!vioc.sound.empty());
	pbRestore.Enable(!vioc.sound.empty());
	pbExtract.Enable(!vioc.sound.empty());
}


//===============================================================================graph-ctrl
VIOCtrl::VIOGraph::VIOGraph(VIOCtrl &par) : vioc(par)
{
	int h=(HS.GetStdSize().cy+GRAPH_HEIGHT);
	SetRect(0,0,100, h+13); //todo: 13 ????
	AddFrame(HS); HS.SetLine(10); HS.Set(0,0,0); HS.WhenScroll << [&]{ Refresh(); };
	ld=rd=0;
	bld=bsel=false;
	b0Line=bGrid=bScales=true;
}

void VIOCtrl::VIOGraph::draw_grid(Draw &drw, int h, int w, Color D, Color DD)
{
	int i, k;
	for (i=5;i<h;i+=5)
	{ //horizontal..
		if ((i%10)==0) { drw.DrawLine(0, h-i, w, h-i, 1, D); drw.DrawLine(0, h+i, w, h+i, 1, D); }
		else { drw.DrawLine(0, h-i, w, h-i, 1, DD); drw.DrawLine(0, h+i, w, h+i, 1, DD); }
	}
	for (i=0;i<w;i++)
	{ //vertical..
		k=(i+HS);
		if ((k%10)==0) drw.DrawLine(i, 0, i, h*2, 1, D);
		else if ((k%5)==0) drw.DrawLine(i, 0, i, h*2, 1, DD);
	}
	drw.DrawLine(0, h, w, h, 1, D);
}

void VIOCtrl::VIOGraph::draw_scales(Draw &drw, int h, int w, Color C)
{
	int i, k;//, m;
	std::string sc="", s;
	auto drawsec=[&](int k)
	{
		s=spf(double(k)/vioc.vio.Rate);
		RTRIM(s,".0");
		sc=spf(s, " sec");
		drw.DrawText(i, (h-12), sc.c_str(), StdFont().Height(8), C);
	};
	for (i=5;i<h;i+=5)
	{
		if ((i%10)==0) { drw.DrawLine(0, h-i, 1, h-i, 3, C); drw.DrawLine(0, h+i, 1, h+i, 3, C); }
		if ((i%50)==0)
		{
			sc=spf(i);  drw.DrawText(5, (h-i-4), sc.c_str(), StdFont().Height(8), C);
			sc=spf("-",i); drw.DrawText(5, (h+i-4), sc.c_str(), StdFont().Height(8), C);
		}
	}
	for (i=0;i<w;i++)
	{
		k=(i+HS);
		if (k>0) { if ((k%500)==0) drawsec(k); }
		if ((k%100)==0) { drw.DrawLine(i, h-3, i, h+4, 3, C); sc=spf(k); drw.DrawText(i, (h+6), sc.c_str(), StdFont().Height(8), C); }
		else if ((k%50)==0) drw.DrawLine(i, h-2, i, h+3, 3, C);
		else if ((k%10)==0) drw.DrawLine(i, h-1, i, h+2, 3, C);
	}
}

void VIOCtrl::VIOGraph::draw_graph(Draw &drw, int h, int w, Color CN, Color CS)
{
	Color col;
	int i, nb, ne, x1, x2, y1, y2, M=((h*2)-1);
	nb=HS; ne=(nb+w); if (size_t(ne)>data.size()) ne=data.size();
	for (i=(nb+1);i<ne;i++)
	{
		x1=(i-nb-2); y1=(M-data[i-1]); x2=(i-nb-1); y2=(M-data[i]); //origin at bottom-left
		col=(bsel&&(i>ld)&&(i<rd))?CS:CN;
		drw.DrawLine(x1, y1, x2, y2, 1, col);
	}
}

void VIOCtrl::VIOGraph::Paint(Draw &drw)
{
	Size sz=GetSize();
	int h=128; //128=>center-line - values are 8-bit (0-256)
	Color gridcol=Color(64,64,0); //dark-yellow
	Color dgridcol=Color(32,32,0); //darker-yellow
	Color selbgcol=Color(10,90,100); //dark-cyan
	Color l0col=Yellow();
	Color scalecol=Yellow();
	Color ldcol=White();
	Color rdcol=White();
	Color mouselinescol=White();
	Color graphnormcol=LtCyan();
	Color graphselcol=White();

	drw.DrawRect(sz, Black());
	if (bsel) drw.DrawRect(ld-HS, 0, rd-ld, sz.cy, selbgcol);
	if (bGrid) draw_grid(drw, h, sz.cx, gridcol, dgridcol);
	if (!data.empty()) draw_graph(drw, h, sz.cx, graphnormcol, graphselcol);
	if (b0Line) drw.DrawLine(0, h, sz.cx, h, 1, l0col);
	if (bScales) draw_scales(drw, h, sz.cx, scalecol);
	if (bsel) { drw.DrawLine(ld-HS, 0, ld-HS, sz.cy, 1, ldcol); drw.DrawLine(rd-HS, 0, rd-HS, sz.cy, 1, rdcol); }
	if (HasMouse()&&!data.empty()) { drw.DrawLine(0, mpt.y, sz.cx, mpt.y, 1, mouselinescol); drw.DrawLine(mpt.x, 0, mpt.x, sz.cy, 1, mouselinescol); }
}

void VIOCtrl::VIOGraph::MouseEnter(Point p, dword keyflags) { prevmouseimg=OverrideCursor((!data.empty())?VIOPICS::mousepoint():Image::Arrow()); }
void VIOCtrl::VIOGraph::MouseLeave() { Refresh(); OverrideCursor(prevmouseimg); }

void VIOCtrl::VIOGraph::RightDown(Point p, dword keyflags)
{
	MenuBar bar;
	Rect r=GetScreenView();
	Point pp(p); pp.x+=r.left; pp.y+=r.top;
	
	bar.Add((bGrid)?"Hide Grid":"Show Grid", THISBACK1(ShowGrid,!bGrid));
	bar.Add((b0Line)?"Hide 0-line":"Show 0-line", THISBACK1(ShowZeroLine,!b0Line));
	bar.Add((bScales)?"Hide Scales":"Show Scales", THISBACK1(ShowScales,!bScales));

	if (WhenRClick) WhenRClick(bar);
	bar.Execute(this, pp);
}

void VIOCtrl::VIOGraph::LeftDown(Point p, dword keyflags)
{
	if (data.empty()) return;
	auto isnear=[](int x, int p){ return ((x>=(p-10))&&(x<=(p+10))); };
	int X=(p.x+HS);
	if (bsel&&(isnear(X,ld)||isnear(X,rd))) { pl=isnear(X,ld); }
	else { ld=rd=X; bsel=false; }
	bld=true;
	SetCapture();
	Refresh();
}
void VIOCtrl::VIOGraph::MouseMove(Point p, dword keyflags)
{
	if (bld)
	{
		int X=(p.x+HS);
		if (p.x>=(GetSize().cx-10)) { HS.Set(HS+20); X+=20; if (X>(int)data.size()) X=(int)data.size(); }
		if (p.x<=10) { HS.Set(HS-20); X-=20; if (X<0) X=0; }
		bsel=true;
		if (X<ld) { ld=X; pl=true; }
		else if (X>rd) { rd=X; pl=false; }
		else if ((X>ld)&&(X<rd)) { if (pl) ld=X; else rd=X; }
		Refresh();
	}
	else { mpt=p; Refresh(); }
}
void VIOCtrl::VIOGraph::LeftUp(Point p, dword keyflags)
{
	bld=false;
	bsel=((rd-ld)>10);
	if (HasCapture()) ReleaseCapture();
	Refresh();
	if (WhenSelection) WhenSelection(); //set tools-status
}

void VIOCtrl::VIOGraph::Plot(const Voice &v, bool bRedraw)
{
	data=v;
	if (bRedraw)
	{
		ld=rd=0;
		bsel=false;
		HS.SetTotal(v.size());
		HS.Set(0);
	}
	Refresh();
}

bool VIOCtrl::VIOGraph::GetSelection(Voice &v)
{
	v.clear();
	if (!data.empty())
	{
		//if ((rd<=data.size())&&(ld<rd)&&((rd-ld)>5)) for (size_t i=ld; i<rd;i++) v+=data[i];
		if ((rd<=(int)data.size())&&(ld<rd)&&((rd-ld)>5)) for (int i=ld; i<rd;i++) v+=data[i];
		else v=data;
	}
	return !v.empty();
}

void VIOCtrl::VIOGraph::SetSelection(int L, int R)
{
	if (!data.empty())
	{
		int l, r, n=data.size();
		if ((L>=0)&&(R>=0))
		{
			if (L<R) { l=L; r=R; } else { l=R; r=L; }
			if (((r-l)>5)&&(l<n)&&(r<n)) { ld=l; rd=r; bsel=true; Refresh(); }
		}
	}
}

//===============================================================================

VIOCtrl::VIOCtrl() : tools(*this), graph(*this)
{
	int hg=graph.GetSize().cy;
	int ht=tools.GetSize().cy;

	ptr_vioctrl=this;
	b_is_recording=false;
	
	SetRect(0,0,100,(hg+ht+9));
	AddFrame(ThinInsetFrame());
	
	Add(graph.HSizePosZ(1, 1).TopPos(1, graph.GetSize().cy));
	graph.WhenRClick = THISBACK(OnMenuBar);
	graph.WhenSelection = THISBACK(on_selection);
	
	Add(tools.HSizePosZ(1,1).TopPos((hg+3), 24));
	tools.WhenRecord = THISBACK(Record);
	tools.WhenPlay = THISBACK(Play);
	tools.WhenPause = THISBACK(Pause);
	tools.WhenStop = THISBACK(Stop);
	tools.volume.WhenSlide = THISBACK(Volume);
	tools.WhenRestore = THISBACK(Restore);
	tools.WhenNew = THISBACK(New);
	tools.WhenExtract = THISBACK(Extract);

	ResetVIO();
}

void VIOCtrl::Layout() { graph.HS.SetPage(GetSize().cx); }

void VIOCtrl::on_selection() { ResetVIO(false); }

void VIOCtrl::volume_setup()
{
	if (!sound.empty())
	{
		VoiceStats SS;
		int v;
		GetVoiceStats(sound, SS);
		v=(int)((double(SS.damplow+128)/256.0)*100.0);
		tools.volume.SetVolume(v, false);
	}
}

void VIOCtrl::ResetVIO(bool bAll/*=true*/)
{
	if (bAll) { graph.Plot(sound); volume_setup(); }
	tools.SetStatus();
	if (b_is_recording&&!vio.IsRecording())
	{
		b_is_recording=false;
		if (WhenRecorded) WhenRecorded(sound);
	}
}

void VIOCtrl::SetVoice(const Voice &v) { sound=v; ResetVIO(); }

void VIOCtrl::Record()
{
	vio.SetVIOCallback(cb_vio_done);
	b_is_recording=true;
	vio.Record(&sound);
	ResetVIO();
}

void VIOCtrl::Play()
{
	if (graph.GetSelection(stemp))
	{
		vio.SetVIOCallback(cb_vio_done);
		vio.Play(&stemp);
	}
	ResetVIO(false);
}

void VIOCtrl::Pause()	{ vio.Pause(); ResetVIO(false); }
void VIOCtrl::Stop()	{ vio.Stop(); ResetVIO(); }

void VIOCtrl::Volume(double dvol) //percentage (0-150)
{
	stemp=sound;
	if (!stemp.empty())
	{
		stemp.SetVolume(dvol);
		graph.Plot(stemp, false);
	}
}

void VIOCtrl::Restore()	{ vio.Stop(); ResetVIO(); }
void VIOCtrl::New()		{ sound.clear(); ResetVIO(); if (WhenNew) WhenNew(); }

void VIOCtrl::Extract()
{
	graph.GetSelection(stemp);
	if (WhenVoice) WhenVoice(stemp);
}

void VIOCtrl::ExtractClip(Voice &v)
{
	graph.GetSelection(v);
}

bool VIOCtrl::GetVoiceStats(const Voice &v, VoiceStats &ss)
{
	
	
/*
	if (us.size()<3) return false;
	size_t i;
	uint8_t p=us[0];

	ss.nInterval=us.size();
	ss.nMaxima=ss.nMinima=0;
	ss.damphigh=ss.damplow=128.0;
	ss.daverage=ss.dmean=ss.dfreq=0.0;
	
	for (i=1;i<(us.size()-1);i++)
	{
		ss.daverage+=us[i];
		if (us[i]>ss.damphigh) ss.damphigh=us[i];
		if (us[i]<ss.damplow) ss.damplow=us[i];
		if ((us[i]>p)&&(us[i]>us[i+1])) { ss.nMaxima++; p=us[i]; }
		if ((us[i]<p)&&(us[i]<us[i+1])) { ss.nMinima++; p=us[i]; }
	}
	ss.daverage/=(ss.nInterval-2); ss.daverage-=128;
	ss.damphigh-=128.0; ss.damplow-=128.0;
	ss.dmean=(ss.damphigh+ss.damplow)/2.0;
	ss.dfreq=(0.5*(ss.nMaxima+ss.nMinima));
	ss.dwavelen=(ss.nInterval/ss.dfreq);
	return true;
*/
return false;
}

