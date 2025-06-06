#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment (lib,"Gdiplus.lib")

using namespace std;


struct monitor
{
	HMONITOR hMon;
	int id;
	RECT lpr;
};

struct mon_setting {
	int id = -1;
	DWORD ori = -1; 
	bool isPrimary = false;
	int x = 0; 
	int y = 0;
};

vector<monitor*> monitors;

static int _id = 0;

void printMonSetting(const mon_setting& ms) {
	std::cout << "Monitor Settings:\n"
		<< "  ID: " << ms.id << "\n"
		<< "  Orientation: " << ms.ori << " (";

	// Расшифровка ориентации
	switch (ms.ori) {
	case DMDO_DEFAULT: std::cout << "DEFAULT"; break;
	case DMDO_90:      std::cout << "90°"; break;
	case DMDO_180:     std::cout << "180°"; break;
	case DMDO_270:     std::cout << "270°"; break;
	default:           std::cout << "UNKNOWN"; break;
	}

	std::cout << ")\n"
		<< "  Primary: " << (ms.isPrimary ? "Yes" : "No") << "\n"
		<< "  Position: (" << ms.x << ", " << ms.y << ")\n"
		<< "----------------------------------\n";
}

HDC getMonHDC(HMONITOR hMon) {
	MONITORINFOEX info;
	info.cbSize = sizeof(MONITORINFOEX);

	GetMonitorInfo(hMon, &info);

	return CreateDC(NULL, info.szDevice, NULL, NULL);
}

void print_monitor_info_yaml(monitor* m) {
	
	cout << "monitor:" << endl;
	cout << "\tid: " << m->id << endl;
	MONITORINFOEX mi;

	mi.cbSize = sizeof(MONITORINFOEX);

	if (!GetMonitorInfo(m->hMon, &mi)) {
		cout << "\terror: \"error is GetMonitorInfo()\"" << endl;
		return;
	}
	wstring name = mi.szDevice;
	wcout << "\tname: \"" << name << "\"" << endl;
	cout << "\tis_primary: " << ((mi.dwFlags == 1) ? "true" : "false") << endl;

	DEVMODE dm = {};
	dm.dmSize = sizeof(DEVMODE);
	if (!EnumDisplaySettingsEx(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm, 0)) {
		cout << "\terror: \"error is EnumDisplaySettingsEx()\"" << endl;
		return;
	}
	cout << "\tposition:\n\t\tx: " << dm.dmPosition.x << "\n\t\ty: " << dm.dmPosition.y << endl;
	cout << "\twidth: " << dm.dmPelsWidth << "\n\theight: " << dm.dmPelsHeight << endl;
	cout << "\torintation: " << dm.dmDisplayOrientation << endl;
}

// Мониторынй итератор
BOOL CALLBACK proc_monitor(
	HMONITOR hMon,
	HDC hdcMon,
	LPRECT lprcMonitor,
	LPARAM lpar
) {
	if (!hMon) return TRUE;

	monitor* m = new monitor;

	m->lpr = *lprcMonitor;
	m->id = _id++;
	m->hMon = hMon;

	monitors.push_back(m);

	return TRUE;
}

bool change_orint_monitor(monitor* m, const mon_setting& ms) {
	MONITORINFOEX mi;
	mi.cbSize = sizeof(MONITORINFOEX);
	wstring n_mon; 
	if (!GetMonitorInfo(m->hMon, &mi)) {
		return false;
	}
	n_mon = mi.szDevice;

	DEVMODE dm = {};
	dm.dmSize = sizeof(DEVMODE);
	if (!EnumDisplaySettingsEx(n_mon.c_str(), ENUM_CURRENT_SETTINGS, &dm, 0))
		return false;
	
	if (ms.ori < 4) {
		DWORD oldOri = dm.dmDisplayOrientation;
		dm.dmDisplayOrientation = ms.ori;
		dm.dmFields = DM_DISPLAYORIENTATION;
		if ((ms.ori + oldOri) % 2) {
			swap(dm.dmPelsWidth, dm.dmPelsHeight);
			dm.dmFields |= DM_PELSWIDTH | DM_PELSHEIGHT;
		}
	}

	if (ms.x != 0) {
		dm.dmPosition.x = ms.x;
	}
	if (ms.y != 0) {
		dm.dmPosition.y = ms.y;
	}
	if (ms.y != 0 || ms.x != 0) {
		dm.dmFields |= DM_POSITION;
	}	

	LONG res;
	if (ms.isPrimary) {
		res = ChangeDisplaySettingsEx(n_mon.c_str(), &dm, NULL, CDS_UPDATEREGISTRY | CDS_RESET | CDS_SET_PRIMARY, NULL);
	}
	else {
		res = ChangeDisplaySettingsEx(n_mon.c_str(), &dm, NULL, CDS_UPDATEREGISTRY | CDS_RESET, NULL);
	}
	if (res != DISP_CHANGE_SUCCESSFUL){
		cout << "allert: " << res << endl;
		return false;
	}
	return true;
}

void pr_help() {

	cout << 
		R"abu(
Monitor Tool v1.0 - Display Configuration Utility

Usage:
  monitor_tool [options]

Options:
  -i          Show monitor info (YAML format)
  -mN         Select monitor ID (0-based)
  -oN         Set orientation (0-3):
               0=Default 1=90° 2=180° 3=270°
  -p          Set as primary monitor
  -s          Set as secondary monitor
  -xN         Set X position
  -yN         Set Y position
  -d          Debug mode (show settings)

Examples:
  monitor_tool -i            # List all monitors
  monitor_tool -m1 -o1 -p    # Rotate monitor 1 to 90° and set as primary
  monitor_tool -m0 -x1920    # Move monitor 0 to X=1920
)abu" << endl;
}

void pr_info() {
	EnumDisplayMonitors(NULL, NULL, proc_monitor, 0);
}

int main(int argc, char* argv[]) {
	mon_setting* mon = new mon_setting;

	pr_info();
	
	for (int i = 0; i < argc; i++) {
		std::string arg = argv[i];

		// print information 
		if (arg.rfind("-i", 0) == 0) {
			for (monitor* m : monitors) {
				print_monitor_info_yaml(m);
			}
		}
		// set orintaion monitor
		if (arg.rfind("-o", 0) == 0) {
			DWORD ori = stoi(arg.substr(2));
			if (ori < 4) {
				mon->ori = ori;
			}
		}
		// primary monitor
		if (arg.rfind("-p", 0) == 0) {
			mon->isPrimary = true;
		}
		// secondary monitor
		if (arg.rfind("-s", 0) == 0) {
			mon->isPrimary = !true;
		}
		// select monitor
		if (arg.rfind("-m", 0) == 0) {
			int id_m = stoi(arg.substr(2));
			if (id_m >= 0 && id_m < monitors.size()) {
				mon->id = id_m;
			}
			else {
				cout << "error monitor id: " << id_m << endl;
				return 2;
			}
		}
		// set x&y position monitor
		if (arg.rfind("-x", 0) == 0) {
			mon->x = stoi(arg.substr(2));
		}

		if (arg.rfind("-y", 0) == 0) {
			mon->y = stoi(arg.substr(2));
		}

		// debug print
		if (arg.rfind("-d", 0) == 0) {
			printMonSetting(*mon);
		}
		if (arg.rfind("-h", 0) == 0) {
			pr_help();
		}
	}
	}
	
	// change setting monitor 
	if (mon->id >= 0 && mon->id < monitors.size())
		change_orint_monitor(monitors[mon->id], *mon);

	delete mon;
}