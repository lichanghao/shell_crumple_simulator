// nano_*.dat reader/writer implementation.
// All Fortran 1-based indices are converted to 0-based on read; written back as 1-based.
// Fortran D-exponent floats ('D'/'d') are normalized to 'E'/'e' before strtod.

#include "fce/io.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace fce {
namespace io {

// ─── Helpers ──────────────────────────────────────────────────────────────────

double parse_fortran_double(const std::string& tok) {
    std::string s = tok;
    for (char& c : s) {
        if (c == 'D' || c == 'd') { c = 'E'; break; }
    }
    return std::stod(s);
}

// Read next non-empty, non-label token (skip lines starting with letters/spaces-then-letters).
// Label lines begin with a non-numeric character (excluding '-').
static bool is_label(const std::string& line) {
    // A line is a label/header if the first non-space character is alphabetic,
    // or if it is a dash-separator line (e.g. " ---------").
    size_t i = 0;
    while (i < line.size() && line[i] == ' ') ++i;
    if (i >= line.size()) return true; // blank line
    char c = line[i];
    if (std::isalpha(static_cast<unsigned char>(c))) return true;
    // Separator line like " ---------" (not a negative number like "-0.5")
    if (c == '-' && i + 1 < line.size() && line[i + 1] == '-') return true;
    return false;
}

// Tokenize a data line (space-separated), parse each token as a double or int.
static std::vector<std::string> tokenize(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

// File-reading context that skips label lines automatically.
struct FileReader {
    std::ifstream f;
    std::string current;

    explicit FileReader(const std::string& path) : f(path) {
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
    }

    // Read next non-blank, non-label data line.
    bool next_data_line() {
        while (std::getline(f, current)) {
            if (!current.empty() && !is_label(current)) return true;
        }
        return false;
    }

    // Read next line regardless of type.
    bool next_line() {
        return static_cast<bool>(std::getline(f, current));
    }

    // Skip until a line that STARTS (trimmed) with the given prefix.
    void skip_to(const std::string& prefix) {
        while (std::getline(f, current)) {
            std::string t = current;
            auto it = t.find_first_not_of(" ");
            if (it != std::string::npos) t = t.substr(it);
            if (t.substr(0, prefix.size()) == prefix) return;
        }
        throw std::runtime_error("Label not found: " + prefix);
    }

    int    read_int()    { next_data_line(); return std::stoi(current); }
    double read_double() { next_data_line(); return parse_fortran_double(current); }

    std::vector<std::string> read_tokens() {
        next_data_line();
        return tokenize(current);
    }
};

// ─── nano_dims.dat ────────────────────────────────────────────────────────────

DimsData read_dims(const std::string& path) {
    FileReader r(path);
    DimsData d;
    d.numele       = r.read_int();
    d.numnods      = r.read_int();
    d.nedge        = r.read_int();
    d.nelem_ghost  = r.read_int();
    d.nnode_ghost  = r.read_int();
    d.ngauss       = r.read_int();
    d.nnodBC       = r.read_int();
    d.ndofBC       = r.read_int();
    d.ndofOP       = r.read_int();
    d.nvdw         = r.read_int();
    if (d.nvdw == 1) {
        d.ngauss_vdw = r.read_int();
        d.ng_tot = r.read_int();
        d.nneigh = r.read_int();
        d.ninrange = r.read_int();
    }
    return d;
}

void write_dims(const std::string& path, const DimsData& d) {
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write: " + path);
    f << " Dims data\n ---------\n";
    f << " mesh0%numele\n"      << std::setw(9) << d.numele      << "\n";
    f << " mesh0%numnods\n"     << std::setw(9) << d.numnods     << "\n";
    f << " mesh0%nedge\n"       << std::setw(9) << d.nedge       << "\n";
    f << " mesh0%nelem_ghost\n" << std::setw(9) << d.nelem_ghost << "\n";
    f << " mesh0%nnode_ghost\n" << std::setw(9) << d.nnode_ghost << "\n";
    f << " ngauss\n"            << std::setw(9) << d.ngauss      << "\n";
    f << " BCs%nnodBC\n"        << std::setw(9) << d.nnodBC      << "\n";
    f << " BCs%ndofBC\n"        << std::setw(9) << d.ndofBC      << "\n";
    f << " BCs%ndofOP\n"        << std::setw(9) << d.ndofOP      << "\n";
    f << " vdw1%nvdw\n"         << std::setw(9) << d.nvdw        << "\n";
    if (d.nvdw == 1) {
        f << " vdw1%ngauss_vdw\n" << std::setw(9) << d.ngauss_vdw << "\n";
        f << " vdw1%ng_tot\n"     << std::setw(9) << d.ng_tot     << "\n";
        f << " nneigh\n"          << std::setw(9) << d.nneigh     << "\n";
        f << " ninrange\n"        << std::setw(9) << d.ninrange   << "\n";
    }
}

// ─── nano_general.dat ─────────────────────────────────────────────────────────

// Write double in Fortran D-exponent format with 17 significant digits.
static std::string fmt_d(double v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%30.17E", v);
    // Replace trailing 'E' marker: Fortran uses D instead of E
    for (int i = static_cast<int>(std::strlen(buf)) - 1; i >= 0; --i) {
        if (buf[i] == 'E') { buf[i] = 'D'; break; }
    }
    // Fortran format: D+XX uses 2-digit exponent; snprintf gives E+00X → need D+0XX
    // The exponent field after 'D' should be sign + 2 digits.
    // Find 'D' position and normalize exponent.
    std::string s(buf);
    auto dpos = s.rfind('D');
    if (dpos != std::string::npos && dpos + 1 < s.size()) {
        std::string exp_part = s.substr(dpos + 1); // e.g. "+002" or "-01"
        char sign = exp_part[0];
        int exp_val = std::stoi(exp_part.substr(1));
        char exp_buf[16];
        std::snprintf(exp_buf, sizeof(exp_buf), "%c%02d", sign, std::abs(exp_val));
        s = s.substr(0, dpos + 1) + std::string(exp_buf);
    }
    return s;
}

#include <iomanip>

GeneralData read_general(const std::string& path) {
    FileReader r(path);
    GeneralData g;
    g.ylength          = r.read_double();
    g.mat.A0           = r.read_double();
    g.mat.nCode_Pot    = r.read_int();
    if (g.mat.nCode_Pot == 1) {
        g.mat.Vs[0] = r.read_double();
        g.mat.Vs[1] = r.read_double();
        g.mat.Va[0] = r.read_double();
        g.mat.Va[1] = r.read_double();
    } else if (g.mat.nCode_Pot == 2) {
        g.mat.A1 = r.read_double();
        g.mat.Vs[0] = r.read_double();
        g.mat.Vs[1] = r.read_double();
        g.mat.Vs[2] = r.read_double();
        g.mat.Va[0] = r.read_double();
        g.mat.Va[1] = r.read_double();
        g.mat.Va[2] = r.read_double();
    } else if (g.mat.nCode_Pot == 3) {
        g.mat.Vs[0] = r.read_double();
        g.mat.Va[0] = r.read_double();
    }
    // Bond vectors E1, E2, E3 (3 rows × 2 columns)
    for (int i = 0; i < 3; ++i) {
        auto toks = r.read_tokens();
        if (toks.size() < 2) throw std::runtime_error("mat1%E row too short");
        g.mat.E[i][0] = parse_fortran_double(toks[0]);
        g.mat.E[i][1] = parse_fortran_double(toks[1]);
    }
    g.mat.s0           = r.read_double();
    g.nW_hat           = (r.read_int() != 0);
    g.crit_global      = r.read_double();
    g.crit_local       = r.read_double();
    g.imperfect        = (r.read_int() != 0);
    g.fact_imp         = r.read_double();
    return g;
}

void write_general(const std::string& path, const GeneralData& g) {
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write: " + path);
    f << " General data\n ------------\n";
    f << " ylength\n"     << fmt_d(g.ylength)    << "\n";
    f << " mat1%A0\n"     << fmt_d(g.mat.A0)     << "\n";
    f << " mat1%nCode_Pot\n" << std::setw(9) << g.mat.nCode_Pot << "\n";
    if (g.mat.nCode_Pot == 1) {
        f << fmt_d(g.mat.Vs[0]) << "\n";
        f << fmt_d(g.mat.Vs[1]) << "\n";
        f << fmt_d(g.mat.Va[0]) << "\n";
        f << fmt_d(g.mat.Va[1]) << "\n";
    } else if (g.mat.nCode_Pot == 2) {
        f << fmt_d(g.mat.A1) << "\n";
        f << fmt_d(g.mat.Vs[0]) << "\n";
        f << fmt_d(g.mat.Vs[1]) << "\n";
        f << fmt_d(g.mat.Vs[2]) << "\n";
        f << fmt_d(g.mat.Va[0]) << "\n";
        f << fmt_d(g.mat.Va[1]) << "\n";
        f << fmt_d(g.mat.Va[2]) << "\n";
    } else if (g.mat.nCode_Pot == 3) {
        f << fmt_d(g.mat.Vs[0]) << "\n";
        f << fmt_d(g.mat.Va[0]) << "\n";
    }
    f << " mat1%E\n";
    for (int i = 0; i < 3; ++i) {
        f << fmt_d(g.mat.E[i][0]) << fmt_d(g.mat.E[i][1]) << "\n";
    }
    f << " mat1%s0\n"    << fmt_d(g.mat.s0)     << "\n";
    f << " nW_hat\n"     << std::setw(9) << (g.nW_hat ? 1 : 0) << "\n";
    f << " crit\n"       << fmt_d(g.crit_global) << "\n"
                         << fmt_d(g.crit_local)  << "\n";
    f << " imperfect\n"  << std::setw(9) << (g.imperfect ? 1 : 0) << "\n";
    f << " fact_imp\n"   << fmt_d(g.fact_imp)    << "\n";
}

// ─── nano_zero.dat ────────────────────────────────────────────────────────────

std::vector<RefConfig> read_zero(const std::string& path, int numele) {
    FileReader r(path);
    int total = numele;
    std::vector<RefConfig> rc(total);
    for (int k = 0; k < total; ++k) {
        rc[k].J0 = r.read_double();
        auto row0 = r.read_tokens();
        if (row0.size() < 2) throw std::runtime_error("nano_zero.dat: F0 row 0 too short");
        rc[k].F0[0][0] = parse_fortran_double(row0[0]);
        rc[k].F0[0][1] = parse_fortran_double(row0[1]);
        auto row1 = r.read_tokens();
        if (row1.size() < 2) throw std::runtime_error("nano_zero.dat: F0 row 1 too short");
        rc[k].F0[1][0] = parse_fortran_double(row1[0]);
        rc[k].F0[1][1] = parse_fortran_double(row1[1]);
    }
    return rc;
}

void write_zero(const std::string& path,
                const std::vector<RefConfig>& rc,
                int numele) {
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write: " + path);
    f << " Zero data\n ------------\n";
    int total = numele;
    for (int k = 0; k < total; ++k) {
        f << fmt_d(rc[k].J0) << "\n";
        f << fmt_d(rc[k].F0[0][0]) << fmt_d(rc[k].F0[0][1]) << "\n";
        f << fmt_d(rc[k].F0[1][0]) << fmt_d(rc[k].F0[1][1]) << "\n";
    }
}

// ─── nano_config.dat ──────────────────────────────────────────────────────────

ConfigData read_config(const std::string& path, int numnods, int numele, int ngauss) {
    FileReader r(path);
    ConfigData c;
    c.coords.resize(numnods);
    // Nodal positions
    for (int i = 0; i < numnods; ++i) {
        auto toks = r.read_tokens();
        if (toks.size() < 3) throw std::runtime_error("nano_config.dat: coords too short");
        c.coords[i][0] = parse_fortran_double(toks[0]);
        c.coords[i][1] = parse_fortran_double(toks[1]);
        c.coords[i][2] = parse_fortran_double(toks[2]);
    }
    // Inner displacement η: [ielem][igauss]
    c.eta.resize(numele, std::vector<Vec2>(ngauss));
    for (int ie = 0; ie < numele; ++ie) {
        for (int ig = 0; ig < ngauss; ++ig) {
            auto toks = r.read_tokens();
            if (toks.size() < 2) throw std::runtime_error("nano_config.dat: eta too short");
            c.eta[ie][ig][0] = parse_fortran_double(toks[0]);
            c.eta[ie][ig][1] = parse_fortran_double(toks[1]);
        }
    }
    return c;
}

void write_config(const std::string& path, const ConfigData& c,
                  int numnods, int numele, int ngauss) {
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write: " + path);
    f << " Config Data\n -----------\n Nodal positions\n";
    for (int i = 0; i < numnods; ++i) {
        f << fmt_d(c.coords[i][0]) << fmt_d(c.coords[i][1]) << fmt_d(c.coords[i][2]) << "\n";
    }
    for (int ie = 0; ie < numele; ++ie) {
        for (int ig = 0; ig < ngauss; ++ig) {
            f << fmt_d(c.eta[ie][ig][0]) << fmt_d(c.eta[ie][ig][1]) << "\n";
        }
    }
}

// ─── nano_BCs.dat ─────────────────────────────────────────────────────────────

// Trim leading/trailing whitespace from a line.
static std::string trim(const std::string& s) {
    auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

BCData read_bcs(const std::string& path) {
    // Sequential scan of the BCs file. We process sections in document order.
    // After a variable-length data loop ends (by hitting a label), we reuse
    // that label rather than searching for it again.
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open: " + path);

    BCData bc;
    std::string line;

    // Helper: advance to a line whose trimmed content equals `label`.
    // Returns the trimmed content of the next data line after the label.
    auto skip_to_label = [&](const std::string& label) {
        while (std::getline(f, line)) {
            if (trim(line) == label) return;
        }
        throw std::runtime_error("BCs label not found: " + label);
    };

    // Helper: read next non-empty line and return trimmed.
    auto next_trimmed = [&]() -> std::string {
        while (std::getline(f, line)) {
            auto t = trim(line);
            if (!t.empty()) return t;
        }
        throw std::runtime_error("Unexpected EOF in " + path);
    };

    skip_to_label("BCs%nloadstep");
    bc.nloadstep = std::stoi(next_trimmed());

    skip_to_label("BCs%nCodeLoad");
    bc.nCodeLoad = std::stoi(next_trimmed());

    // mdofBC — read until next label
    skip_to_label("BCs%mdofBC");
    while (std::getline(f, line)) {
        auto t = trim(line);
        if (t.empty()) continue;
        if (is_label(t)) {
            // t is next section label; handle it below
            goto after_mdofBC;
        }
        bc.mdofBC.push_back(std::stoi(t) - 1);
    }
    after_mdofBC:
    // `line` now holds "BCs%mdofOP" or similar label

    // mdofOP — already positioned at its label
    // If not the right label, search forward
    if (trim(line) != "BCs%mdofOP") skip_to_label("BCs%mdofOP");
    while (std::getline(f, line)) {
        auto t = trim(line);
        if (t.empty()) continue;
        if (is_label(t)) goto after_mdofOP;
        bc.mdofOP.push_back(std::stoi(t) - 1);
    }
    after_mdofOP:

    // mnodBC
    if (trim(line) != "BCs%mnodBC") skip_to_label("BCs%mnodBC");
    while (std::getline(f, line)) {
        auto t = trim(line);
        if (t.empty()) continue;
        if (is_label(t)) goto after_mnodBC;
        auto toks = tokenize(line);
        if (toks.size() >= 2) {
            std::array<int,2> pair{std::stoi(toks[0]) - 1, std::stoi(toks[1]) - 1};
            bc.mnodBC.push_back(pair);
        }
    }
    after_mnodBC:

    // rotation 3×3 — already at "BCs%rotation" label (or search)
    if (trim(line) != "BCs%rotation") skip_to_label("BCs%rotation");
    for (int row = 0; row < 3; ++row) {
        if (!std::getline(f, line)) throw std::runtime_error("Short rotation in " + path);
        auto toks = tokenize(line);
        for (int col = 0; col < 3; ++col)
            bc.rotation[row][col] = parse_fortran_double(toks[col]);
    }

    // xc
    skip_to_label("BCs%xc");
    if (!std::getline(f, line)) throw std::runtime_error("Short xc in " + path);
    { auto toks = tokenize(line);
      for (int i = 0; i < 3; ++i) bc.xc[i] = parse_fortran_double(toks[i]); }

    // value
    skip_to_label("BCs%value");
    bc.value = parse_fortran_double(next_trimmed());

    // Cyclic params are present in the archived compression/cyclic files written
    // by the newer preprocessor path, but older non-cyclic Fortran archives stop
    // after BCs%value. Default to the non-cyclic equivalents when the tail is absent.
    try {
        skip_to_label("BCs%ncycles");
        bc.ncycles = std::stoi(next_trimmed());
        skip_to_label("BCs%nloadstep_comp");
        bc.nloadstep_comp = std::stoi(next_trimmed());
        skip_to_label("BCs%nloadstep_rel");
        bc.nloadstep_rel = std::stoi(next_trimmed());
        skip_to_label("BCs%value_comp");
        bc.value_comp = parse_fortran_double(next_trimmed());
        skip_to_label("BCs%value_rel");
        bc.value_rel = parse_fortran_double(next_trimmed());
    } catch (const std::runtime_error&) {
        bc.ncycles = 1;
        bc.nloadstep_comp = bc.nloadstep;
        bc.nloadstep_rel = 0;
        bc.value_comp = bc.value;
        bc.value_rel = 0.0;
    }

    bc.ndofBC = static_cast<int>(bc.mdofBC.size());
    bc.ndofOP = static_cast<int>(bc.mdofOP.size());
    bc.nnodBC = static_cast<int>(bc.mnodBC.size());
    return bc;
}

void write_bcs(const std::string& path, const BCData& bc) {
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write: " + path);
    f << " BCs data\n --------\n";
    f << " BCs%nloadstep\n" << std::setw(9) << bc.nloadstep << "\n";
    f << " BCs%nCodeLoad\n" << std::setw(9) << bc.nCodeLoad << "\n";
    f << " BCs%mdofBC\n";
    for (int v : bc.mdofBC) f << std::setw(9) << (v + 1) << "\n"; // 0→1
    f << " BCs%mdofOP\n";
    for (int v : bc.mdofOP) f << std::setw(9) << (v + 1) << "\n";
    f << " BCs%mnodBC\n";
    for (auto& p : bc.mnodBC) f << std::setw(9) << (p[0]+1) << std::setw(9) << (p[1]+1) << "\n";
    f << " BCs%rotation\n";
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) f << fmt_d(bc.rotation[row][col]);
        f << "\n";
    }
    f << " BCs%xc\n";
    for (int i = 0; i < 3; ++i) f << fmt_d(bc.xc[i]);
    f << "\n";
    f << " BCs%value\n" << fmt_d(bc.value) << "\n";
    f << " BCs%ncycles\n"        << std::setw(9) << bc.ncycles        << "\n";
    f << " BCs%nloadstep_comp\n" << std::setw(9) << bc.nloadstep_comp << "\n";
    f << " BCs%nloadstep_rel\n"  << std::setw(9) << bc.nloadstep_rel  << "\n";
    f << " BCs%value_comp\n"  << fmt_d(bc.value_comp) << "\n";
    f << " BCs%value_rel\n"   << fmt_d(bc.value_rel)  << "\n";
}

// ─── nano_Mesh.dat ────────────────────────────────────────────────────────────

Mesh read_mesh(const std::string& path, int ngauss) {
    (void)ngauss;
    Mesh m;
    int max_vertex = -1;
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open: " + path);
    std::string line;

    // Skip 3 header lines: "Mesh data", "---------", "Connect"
    for (int i = 0; i < 3; ++i) std::getline(f, line);

    // Read elements until nghost_tab label
    while (true) {
        if (!std::getline(f, line)) break;
        std::string t = line;
        auto p = t.find_first_not_of(" ");
        if (p != std::string::npos) t = t.substr(p);
        if (t == "nghost_tab") break;
        if (t != "New element") continue;

        TriElement el;
        // ielem v1 v2 v3
        std::getline(f, line);
        {auto toks = tokenize(line);
        // ielem is 1-based (ignore for ordering); vertices 1-based → 0-based
        el.vertices[0] = std::stoi(toks[1]) - 1;
        el.vertices[1] = std::stoi(toks[2]) - 1;
        el.vertices[2] = std::stoi(toks[3]) - 1;}
        max_vertex = std::max({max_vertex, el.vertices[0], el.vertices[1], el.vertices[2]});
        // num_neigh_elem
        std::getline(f, line); el.num_neigh_elem = std::stoi(line);
        // num_neigh_vert
        std::getline(f, line); el.num_neigh_vert = std::stoi(line);
        // 12 pairs: col0 = neighboring element index (1-based, 0 if none), col1 = node index (1-based, 0 if absent)
        for (int j = 0; j < 12; ++j) {
            std::getline(f, line);
            auto toks = tokenize(line);
            el.neigh_elem[j] = std::stoi(toks[0]);
            int ni = std::stoi(toks[1]);
            el.neigh_vert[j] = (ni > 0) ? (ni - 1) : -1; // convert 1-based → 0-based (real and ghost alike)
        }
        // code_bc(1:3)
        std::getline(f, line);
        {auto toks = tokenize(line);
        el.code_bc[0] = std::stoi(toks[0]);
        el.code_bc[1] = std::stoi(toks[1]);
        el.code_bc[2] = std::stoi(toks[2]);}

        m.connect.push_back(el);
    }
    m.numele = static_cast<int>(m.connect.size());
    m.numnods = max_vertex + 1;

    // nghost_tab: nedge rows of 3 integers (1-based node indices → 0-based)
    while (std::getline(f, line)) {
        std::string t = line;
        auto p = t.find_first_not_of(" ");
        if (p != std::string::npos) t = t.substr(p);
        if (t == "nelem_ghost") break;
        auto toks = tokenize(line);
        if (toks.size() < 3) continue;
        std::vector<int> row = {std::stoi(toks[0]) - 1,
                                 std::stoi(toks[1]) - 1,
                                 std::stoi(toks[2]) - 1};
        m.nghost_tab.push_back(row);
    }
    m.nedge = static_cast<int>(m.nghost_tab.size());

    // nelem_ghost rows
    while (std::getline(f, line)) {
        std::string t = line;
        auto p = t.find_first_not_of(" ");
        if (p != std::string::npos) t = t.substr(p);
        if (t == "nnode_ghost") break;
        auto toks = tokenize(line);
        if (!toks.empty()) m.elem_ghost.push_back(std::stoi(toks[0]) - 1);
    }
    m.nelem_ghost = static_cast<int>(m.elem_ghost.size());

    // nnode_ghost rows
    while (std::getline(f, line)) {
        auto toks = tokenize(line);
        if (!toks.empty()) m.node_ghost.push_back(std::stoi(toks[0]) - 1);
    }
    m.nnode_ghost = static_cast<int>(m.node_ghost.size());

    return m;
}

void write_mesh(const std::string& path, const Mesh& mesh, int ngauss) {
    (void)ngauss;
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write: " + path);
    f << " Mesh data\n ---------\n Connect\n";
    for (int ie = 0; ie < mesh.numele; ++ie) {
        const auto& el = mesh.connect[ie];
        f << " New element\n";
        f << std::setw(9) << (ie + 1)
          << std::setw(9) << (el.vertices[0] + 1)
          << std::setw(9) << (el.vertices[1] + 1)
          << std::setw(9) << (el.vertices[2] + 1) << "\n";
        f << std::setw(9) << el.num_neigh_elem << "\n";
        f << std::setw(9) << el.num_neigh_vert << "\n";
        for (int j = 0; j < 12; ++j) {
            int gflag = el.neigh_elem[j];
            int ni    = el.neigh_vert[j];
            // Convert back: real nodes (gflag==0, ni>=0) go back to 1-based
            int ni_out = (gflag == 0 && ni >= 0) ? (ni + 1) : ni;
            f << std::setw(9) << gflag << std::setw(9) << ni_out << "\n";
        }
        f << std::setw(9) << el.code_bc[0]
          << std::setw(9) << el.code_bc[1]
          << std::setw(9) << el.code_bc[2] << "\n";
    }
    f << " nghost_tab\n";
    for (const auto& row : mesh.nghost_tab) {
        f << std::setw(6) << (row[0] + 1)
          << std::setw(6) << (row[1] + 1)
          << std::setw(6) << (row[2] + 1) << "\n";
    }
    f << " nelem_ghost\n";
    for (int v : mesh.elem_ghost) f << std::setw(6) << (v + 1) << std::setw(6) << 0 << std::setw(6) << 0 << "\n";
    f << " nnode_ghost\n";
    for (int v : mesh.node_ghost) f << std::setw(6) << (v + 1) << std::setw(6) << 0 << std::setw(6) << 0 << "\n";
}

// ─── nano_tub_loc.dat ─────────────────────────────────────────────────────────

std::vector<std::pair<int,int>> read_tub_loc(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open: " + path);
    std::vector<std::pair<int,int>> result;
    std::string line;
    int nranks = 0;
    // First line: number of ranks
    while (std::getline(f, line)) {
        auto toks = tokenize(line);
        if (!toks.empty()) { nranks = std::stoi(toks[0]); break; }
    }
    for (int r = 0; r < nranks; ++r) {
        // Each entry: single integer (ielem_end, 1-based)
        // From the oracle: "1\n      150400" → nranks=1, ielem_end=150400?
        // Actually looking at nano_tub_loc.dat: "1\n      150400"
        // This appears to be: nranks=1, then end_index=150400 (which is nelem*ngauss?)
        // Let's read the format from Fortran source.
        while (std::getline(f, line)) {
            auto toks = tokenize(line);
            if (!toks.empty()) {
                // The value is 1-based end index; start for rank 0 is 1.
                // For rank r: istart=previous+1, iend=this
                int iend_1based = std::stoi(toks[0]);
                int istart_0based = result.empty() ? 0 : result.back().second;
                result.emplace_back(istart_0based, iend_1based - 1); // convert to 0-based closed?
                // Actually: store as 0-based [istart, iend) half-open
                // iend_1based is inclusive 1-based, so iend_0based = iend_1based - 1 (inclusive 0-based)
                // half-open: [istart_0based, iend_1based)
                result.back() = {istart_0based, iend_1based};
                break;
            }
        }
    }
    return result;
}

void write_tub_loc(const std::string& path,
                   const std::vector<std::pair<int,int>>& parts) {
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write: " + path);
    f << std::setw(9) << static_cast<int>(parts.size()) << "\n";
    for (const auto& p : parts) {
        f << std::setw(9) << p.second << "\n"; // 0-based exclusive end → 1-based inclusive end
    }
}

// ─── nano_vdw.dat ─────────────────────────────────────────────────────────────

VdwData read_vdw(const std::string& path, int ng_tot, int ngauss_vdw, int nneigh) {
    FileReader r(path);
    VdwData vdw;
    vdw.nvdw = 1;
    vdw.ng_tot = ng_tot;
    vdw.ngauss_vdw = ngauss_vdw;
    vdw.nneigh = nneigh;

    vdw.meval = r.read_int();
    vdw.r_cut = r.read_double();
    vdw.r_bond = r.read_double();
    vdw.sig = r.read_double();
    vdw.a = r.read_double();
    vdw.y0 = r.read_double();
    {
        const auto toks = r.read_tokens();
        if (toks.size() < 2) throw std::runtime_error("nano_vdw.dat: Vcut row too short");
        vdw.Vcut[0] = parse_fortran_double(toks[0]);
        vdw.Vcut[1] = parse_fortran_double(toks[1]);
    }
    vdw.xc0 = r.read_double();
    vdw.yc0 = r.read_double();

    vdw.shapef.assign(ngauss_vdw, std::array<double, 12>{});
    for (int inode = 0; inode < 12; ++inode) {
        const auto toks = r.read_tokens();
        if (static_cast<int>(toks.size()) < ngauss_vdw) {
            throw std::runtime_error("nano_vdw.dat: shapef row too short");
        }
        for (int ig = 0; ig < ngauss_vdw; ++ig) {
            vdw.shapef[ig][inode] = parse_fortran_double(toks[ig]);
        }
    }

    {
        const auto toks = r.read_tokens();
        if (static_cast<int>(toks.size()) < ngauss_vdw) {
            throw std::runtime_error("nano_vdw.dat: weight row too short");
        }
        vdw.weight.resize(ngauss_vdw);
        for (int ig = 0; ig < ngauss_vdw; ++ig) {
            vdw.weight[ig] = parse_fortran_double(toks[ig]);
        }
    }

    vdw.rho.reserve(ng_tot);
    while (static_cast<int>(vdw.rho.size()) < ng_tot && r.next_data_line()) {
        const auto toks = tokenize(r.current);
        for (const auto& tok : toks) {
            vdw.rho.push_back(parse_fortran_double(tok));
            if (static_cast<int>(vdw.rho.size()) == ng_tot) {
                break;
            }
        }
    }
    if (static_cast<int>(vdw.rho.size()) != ng_tot) {
        throw std::runtime_error("nano_vdw.dat: rho payload size mismatch");
    }

    if (nneigh > 0) {
        vdw.near.assign(ng_tot, std::vector<int>(nneigh + 1, 0));
        for (int i = 0; i < ng_tot; ++i) {
            const auto toks = r.read_tokens();
            if (static_cast<int>(toks.size()) < nneigh + 1) {
                throw std::runtime_error("nano_vdw.dat: near row too short");
            }
            for (int j = 0; j <= nneigh; ++j) {
                vdw.near[i][j] = std::stoi(toks[j]);
            }
        }
    }

    return vdw;
}

void write_vdw(const std::string& path, const VdwData& vdw) {
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write: " + path);

    f << " vdw1%meval\n" << std::setw(11) << vdw.meval << "\n";
    f << " vdw1%r_cut\n" << fmt_d(vdw.r_cut) << "\n";
    f << " vdw1%r_bond\n" << fmt_d(vdw.r_bond) << "\n";
    f << " vdw1%sig\n" << fmt_d(vdw.sig) << "\n";
    f << " vdw1%a\n" << fmt_d(vdw.a) << "\n";
    f << " vdw1%y0\n" << fmt_d(vdw.y0) << "\n";
    f << " vdw1%Vcut(2)\n" << fmt_d(vdw.Vcut[0]) << fmt_d(vdw.Vcut[1]) << "\n";
    f << " vdw1%xc0\n" << fmt_d(vdw.xc0) << "\n";
    f << " vdw1%yc0\n" << fmt_d(vdw.yc0) << "\n";
    f << " vdw1%shapef\n";
    for (int inode = 0; inode < 12; ++inode) {
        for (int ig = 0; ig < vdw.ngauss_vdw; ++ig) {
            f << fmt_d(vdw.shapef[ig][inode]);
        }
        f << "\n";
    }
    f << " vdw1%weight\n";
    for (double weight : vdw.weight) {
        f << fmt_d(weight);
    }
    f << "\n";
    f << " vdw1%rho\n";
    for (int i = 0; i < static_cast<int>(vdw.rho.size()); ++i) {
        if (i > 0 && i % 10 == 0) {
            f << "\n";
        }
        f << fmt_d(vdw.rho[i]);
    }
    f << "\n";
    f << " vdw1%near\n";
    if (vdw.nneigh > 0) {
        for (const auto& row : vdw.near) {
            for (int value : row) {
                f << std::setw(12) << value;
            }
            f << "\n";
        }
    }
}

// ─── nano_crease.dat ──────────────────────────────────────────────────────────

CreaseData read_crease(const std::string& path, int numnods, int ngauss) {
    (void)numnods;
    FileReader r(path);
    CreaseData c;
    c.ncrease = r.read_int();
    c.kappa_cr = r.read_double();
    c.alpha_lock = r.read_double();
    (void)ngauss;
    return c;
}

void write_crease(const std::string& path, const CreaseData& c,
                  int numnods, int ngauss) {
    (void)numnods;
    (void)ngauss;
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write: " + path);
    f << " ncrease\n";
    f << std::setw(12) << c.ncrease << "\n";
    f << " kappa_cr\n";
    f << fmt_d(c.kappa_cr) << "\n";
    f << " alpha_lock\n";
    f << fmt_d(c.alpha_lock) << "\n";
}

} // namespace io
} // namespace fce
