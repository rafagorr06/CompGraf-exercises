#include <fstream>
#include <sstream>
#include <vector>
#include <limits>
using namespace std;

void getBB(const char *name, double bb[3][2]) {
	ifstream fin(name);
	for (int i=0; i<3; ++i) {
		bb[i][0] = numeric_limits<double>::max();
		bb[i][1] = numeric_limits<double>::lowest();
	}
	for (string s; getline(fin,s); ) {
		if (s.find("v ")==0) {
			stringstream ssi(s), sso;
			char c; double v[3];
			ssi>>c>>v[0]>>v[1]>>v[2];
			for (int i=0; i<3; ++i) {
				bb[i][0] = min(bb[i][0],v[i]);
				bb[i][1] = max(bb[i][1],v[i]);
			}
		}
	}
}

int main() {
//	double bbl[3][2],bbh[3][2];
//	getBB("chookity.ld",bbl);
//	getBB("chookity.hd",bbh);
	
	vector<std::string> vt;
	ifstream fin("chookity.hd");
	ofstream fout("chookity.obj");
	for (string s; getline(fin,s); ) {
		
		if (s.find("v ")==0) {
			stringstream ssi(s), sso;
			char c; double v[3], s, t;
			ssi>>c>>v[0]>>v[1]>>v[2];
			
			double v0[] = {1.727671,1.026901,-0.133944};
			double vt0[] = {0.051562,0.706161};
			double v1[] = {1.736937,0.937606,-0.223239};
			double vt1[] = {0.049204,0.688405};
			auto lerp = [&](int i1, int i2) {
				double a = (v[i1]-v0[i1])/(v1[i1]-v0[i1]);
				return (1-a)*vt0[i2] + a*vt1[i2];
			};
			
			s = lerp(0,0)+0.01;
			t = lerp(1,1);//;*1.03-0.025;
			sso<<"vt "<<s<<' '<<t;
			vt.push_back(sso.str());
		} else if (!vt.empty() and s.find("vn ")==0) {
			for (string t : vt)
				fout << t << endl;
			vt.clear();
		} else if (s.find("f ")==0) {
			
			for(char &c:s) if (c=='/') c=' ';
			char c; int v[3][2];
			stringstream ssi(s), sso;
			ssi>>c >> v[0][0]>>v[0][1] >> v[1][0]>>v[1][1] >> v[2][0]>>v[2][1];
			sso << "f "<< v[0][0]<<'/'<<v[0][0]<<'/'<<v[0][1] << ' ' << v[1][0]<<'/'<<v[1][0]<<'/'<<v[1][1] << ' ' << v[2][0]<<'/'<<v[2][0]<<'/'<<v[2][1];
			s = sso.str();
		}
		fout << s << endl;
	}
}
