// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//                 ORBITER MODULE: Framerate
//                  Part of the ORBITER SDK
//
// Framerate.cpp
// Dialog box for displaying simulation frame rate.
// ==============================================================

#define ORBITER_MODULE
#include "Orbitersdk.h"
#include "imgui.h"
#include "implot.h"
#define NDATA 256

// ==============================================================
// The module interface class - singleton implementation

namespace oapi {

	/// \brief Plugin for graphically displaying frame rate and time step length.
	class Framerate : public Module, ImGuiDialog {
	public:
		/// \brief Soliton instance server for Framerate plugin
		/// \param hDLL nodule instance handle
		static Framerate* GetInstance(HINSTANCE hDLL);

		/// \brief Soliton instance destructor
		static void DelInstance();

		/// \brief Entry point for open dialog callback
		static void hookOpenDlg(void* context);

		/// \brief Open dialog callback
		void clbkOpenDlg(void* context);

		/// \brief Time step notification callback
		void clbkPreStep(double simt, double simdt, double mjd);

		void OnDraw();

	protected:
		/// \brief Protected constructor
		Framerate(HINSTANCE hDLL);

		/// \brief Protected destructor
		~Framerate();

		void InsertData(float fps, float dt);
	private:
		static Framerate* self;  ///> Soliton instance pointer
		DWORD m_dwCmd;           ///> Handle for plugin entry in custom command list

		double m_sysT;           ///> current system time
		double m_simT;           ///> current simulation time
		double m_DT;             ///> sample interval
		DWORD m_fcount;          ///> frame counter

		std::vector<float> m_FPS;
		std::vector<float> m_DTPS;
		int m_idx;
	};

} // namespace oapi


// ==============================================================
// API interface
// ==============================================================

/// \brief Module entry point 
/// \param hDLL module handle
DLLCLBK void InitModule (HINSTANCE hDLL)
{
	// Create and register the module
	oapiRegisterModule(oapi::Framerate::GetInstance(hDLL));
}

/// \brief Module exit point 
/// \param hDLL module handle
DLLCLBK void ExitModule (HINSTANCE hDLL)
{
	// Delete the module
	oapi::Framerate::DelInstance();
}


// ==============================================================
// Framerate module interface class
// ==============================================================

oapi::Framerate* oapi::Framerate::self = nullptr;

// --------------------------------------------------------------

oapi::Framerate* oapi::Framerate::GetInstance(HINSTANCE hDLL)
{
	if (!self)
		self = new Framerate(hDLL);
	return self;
}

// --------------------------------------------------------------

void oapi::Framerate::DelInstance()
{
	if (self) {
		delete self;
		self = nullptr;
	}
}

// --------------------------------------------------------------

oapi::Framerate::Framerate(HINSTANCE hDLL)
	: Module(hDLL), ImGuiDialog("Orbiter Performance Meter", {500,350})
{
	// Register the custom command for the plugin
	static char* desc = (char*)"Simulation frame rate / time step monitor";
	m_dwCmd = oapiRegisterCustomCmd((char*)"Performance Meter", desc, hookOpenDlg, NULL);

	m_sysT = 0.0;
	m_simT = 0.0;
	m_DT = 0.1;
	m_fcount = 0;

	m_DTPS.resize(NDATA);
	m_FPS.resize(NDATA);
	m_idx = 0;
}

void oapi::Framerate::InsertData(float fps, float dt)
{
	m_FPS[m_idx] = fps;
	m_DTPS[m_idx] = dt;
	int idx = (m_idx + 1) % NDATA;
	m_idx = idx;
}

void oapi::Framerate::OnDraw()
{
    if (ImPlot::BeginPlot("Performance Meter", ImVec2(-1,0), ImPlotFlags_NoTitle|ImPlotFlags_NoMenus)) {
        ImPlot::SetupAxis(ImAxis_X1, NULL, ImPlotAxisFlags_None);
		ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0, NDATA);

        ImPlot::SetupAxis(ImAxis_Y1, "F/s", ImPlotAxisFlags_None);
		ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0, 10000.0);
        ImPlot::SetupAxis(ImAxis_Y2, "dt/s", ImPlotAxisFlags_AuxDefault);
		ImPlot::SetupAxisScale(ImAxis_Y2, ImPlotScale_Log10);
		ImPlot::SetupAxisLimitsConstraints(ImAxis_Y2, 0.0001, 10000.0);

		ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
        ImPlot::PlotLine("FPS", m_FPS.data(), NDATA, 1.0, 0.0, ImPlotLineFlags_None, m_idx);
        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
        ImPlot::PlotLine("dt", m_DTPS.data(), NDATA, 1.0, 0.0, ImPlotLineFlags_None, m_idx);
        ImPlot::EndPlot();
    }
}


// --------------------------------------------------------------

oapi::Framerate::~Framerate()
{
	// Unregister the window class for the graph
	UnregisterClass("PerfGraphWindow", GetModule());

	// Unregister the custom command for calling the plugin
	oapiUnregisterCustomCmd(m_dwCmd);
}

// --------------------------------------------------------------
// Per-frame update

void oapi::Framerate::clbkPreStep(double simt, double simdt, double mjd)
{
	double syst = oapiGetSysTime(); // ignore time acceleration for graph updates
	m_fcount++;

	if (syst >= m_sysT + m_DT) {
		float fps = (float)(m_fcount / (syst - m_sysT));
		float dt = (float)(simt - m_simT) / (float)m_fcount;
		InsertData(fps, dt);
		m_sysT = syst;
		m_simT = simt;
		m_fcount = 0;
	}
}

// --------------------------------------------------------------

void oapi::Framerate::hookOpenDlg(void* context)
{
	self->clbkOpenDlg(context);
}

// --------------------------------------------------------------

void oapi::Framerate::clbkOpenDlg(void* context)
{
	oapiOpenDialog(self);
}
