#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

constexpr double INF = 1e100;

std::string trim(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    const auto b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::vector<std::string> split_ws(const std::string& s) {
    std::istringstream is(s);
    std::vector<std::string> result;
    std::string token;
    while (is >> token) result.push_back(token);
    return result;
}

double parse_double(const std::string& s) {
    return std::stod(s);
}

std::string number(double x) {
    if (x >= INF / 2) return "+inf";
    if (x <= -INF / 2) return "-inf";
    if (std::fabs(x) < 5e-13) x = 0.0;

    std::ostringstream os;
    os << std::setprecision(12) << x;
    return os.str();
}

struct Row {
    char sense = 'N';  // N objective/free row, L <=, G >=, E =
};

struct Var {
    double lb = 0.0;
    double ub = INF;
    bool integer = false;
};

struct Mps {
    std::string name;
    std::string obj_row;  // empty means no objective row was found
    std::vector<std::string> row_order;
    std::vector<std::string> var_order;
    std::unordered_map<std::string, Row> rows;
    std::unordered_map<std::string, Var> vars;
    std::map<std::string, std::map<std::string, double>> matrix;  // row -> var -> coefficient
    std::unordered_map<std::string, double> rhs;
};

void add_var(Mps& m, const std::string& var) {
    if (!m.vars.count(var)) {
        m.vars[var] = Var{};
        m.var_order.push_back(var);
    }
}

void add_coef(Mps& m, const std::string& var, const std::string& row, double coef) {
    add_var(m, var);
    m.matrix[row][var] += coef;
}

Mps read_mps(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open MPS file: " + path);
    Mps m;
    std::string line;
    std::string section;
    bool in_integer_block = false;

    while (std::getline(in, line)) {
        const std::string raw = line;
        line = trim(line);
        if (line.empty() || line[0] == '*') continue;

        const auto tokens = split_ws(line);
        if (tokens.empty()) continue;
        const std::string& key = tokens[0];

        if (key == "NAME") {
            if (tokens.size() > 1) m.name = tokens[1];
            continue;
        }
        if (key == "ROWS" || key == "COLUMNS" || key == "RHS" ||
            key == "BOUNDS" || key == "RANGES") {
            section = key;
            continue;
        }
        if (key == "ENDATA") break;

        if (section == "ROWS") {
            if (tokens.size() < 2) throw std::runtime_error("Bad ROWS line: " + raw);
            const char sense = tokens[0][0];
            const std::string& row = tokens[1];
            m.rows[row] = Row{sense};
            m.row_order.push_back(row);
            m.rhs[row] = 0.0;
            if (sense == 'N' && m.obj_row.empty()) m.obj_row = row;
        } else if (section == "COLUMNS") {
            if (tokens.size() >= 3 && tokens[1].find("MARKER") != std::string::npos) {
                if (tokens[2].find("INTORG") != std::string::npos) in_integer_block = true;
                if (tokens[2].find("INTEND") != std::string::npos) in_integer_block = false;
                continue;
            }
            if (tokens.size() < 3) throw std::runtime_error("Bad COLUMNS line: " + raw);
            const std::string& var = tokens[0];
            add_var(m, var);
            if (in_integer_block) m.vars[var].integer = true;
            for (std::size_t i = 1; i + 1 < tokens.size(); i += 2) {
                add_coef(m, var, tokens[i], parse_double(tokens[i + 1]));
            }
        } else if (section == "RHS") {
            if (tokens.size() < 3) throw std::runtime_error("Bad RHS line: " + raw);
            for (std::size_t i = 1; i + 1 < tokens.size(); i += 2) {
                m.rhs[tokens[i]] = parse_double(tokens[i + 1]);
            }
        } else if (section == "BOUNDS") {
            if (tokens.size() < 3) throw std::runtime_error("Bad BOUNDS line: " + raw);
            const std::string& type = tokens[0];
            const std::string& var = tokens[2];
            add_var(m, var);
            Var& v = m.vars[var];

            if (type == "LO") {
                if (tokens.size() < 4) throw std::runtime_error("LO missing value: " + raw);
                v.lb = parse_double(tokens[3]);
            } else if (type == "UP") {
                if (tokens.size() < 4) throw std::runtime_error("UP missing value: " + raw);
                v.ub = parse_double(tokens[3]);
            } else if (type == "FX") {
                if (tokens.size() < 4) throw std::runtime_error("FX missing value: " + raw);
                v.lb = v.ub = parse_double(tokens[3]);
            } else if (type == "FR") {
                v.lb = -INF;
                v.ub = INF;
            } else if (type == "MI") {
                v.lb = -INF;
            } else if (type == "PL") {
                v.ub = INF;
            } else if (type == "BV") {
                v.integer = true;
                v.lb = 0.0;
                v.ub = 1.0;
            } else if (type == "LI") {
                if (tokens.size() < 4) throw std::runtime_error("LI missing value: " + raw);
                v.integer = true;
                v.lb = parse_double(tokens[3]);
            } else if (type == "UI") {
                if (tokens.size() < 4) throw std::runtime_error("UI missing value: " + raw);
                v.integer = true;
                v.ub = parse_double(tokens[3]);
            } else {
                throw std::runtime_error("Unsupported BOUNDS type: " + type);
            }
        } else if (section == "RANGES") {
            throw std::runtime_error("RANGES section is not implemented; convert ranges to two rows first.");
        }
    }

    return m;
}

struct Aux {
    std::vector<std::string> lower_vars;
    std::vector<std::string> lower_rows;
    std::unordered_set<std::string> lower_var_set;
    std::unordered_set<std::string> lower_row_set;
    std::map<std::string, double> lower_obj;
};

Aux read_aux(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open AUX file: " + path);
    Aux aux;
    std::string line;
    std::string section;
    int expected_vars = -1;
    int expected_rows = -1;

    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;

        if (line[0] == '@') {
            section = line;
            if (section == "@NUMVARS") {
                if (!std::getline(in, line)) throw std::runtime_error("Missing value after @NUMVARS");
                expected_vars = std::stoi(trim(line));
                section.clear();
            } else if (section == "@NUMCONSTRS") {
                if (!std::getline(in, line)) throw std::runtime_error("Missing value after @NUMCONSTRS");
                expected_rows = std::stoi(trim(line));
                section.clear();
            }
            continue;
        }

        const auto tokens = split_ws(line);
        if (tokens.empty()) continue;

        if (section == "@VARSBEGIN") {
            aux.lower_vars.push_back(tokens[0]);
            aux.lower_var_set.insert(tokens[0]);
            if (tokens.size() >= 2) aux.lower_obj[tokens[0]] = parse_double(tokens[1]);
        } else if (section == "@CONSTRSBEGIN") {
            aux.lower_rows.push_back(tokens[0]);
            aux.lower_row_set.insert(tokens[0]);
        }
    }

    if (expected_vars >= 0 && static_cast<int>(aux.lower_vars.size()) != expected_vars) {
        throw std::runtime_error("AUX @NUMVARS does not match @VARSBEGIN entries.");
    }
    if (expected_rows >= 0 && static_cast<int>(aux.lower_rows.size()) != expected_rows) {
        throw std::runtime_error("AUX @NUMCONSTRS does not match @CONSTRSBEGIN entries.");
    }
    return aux;
}

std::string linear_expr(const std::map<std::string, double>& coefs,
                        const std::vector<std::string>& order) {
    std::string result;
    bool first = true;
    for (const std::string& var : order) {
        const auto it = coefs.find(var);
        if (it == coefs.end()) continue;
        double coef = it->second;
        if (std::fabs(coef) < 5e-13) continue;

        if (first) {
            if (coef < 0) result += "- ";
        } else {
            result += (coef < 0 ? " - " : " + ");
        }

        const double abs_coef = std::fabs(coef);
        if (std::fabs(abs_coef - 1.0) > 5e-13) result += number(abs_coef) + " ";
        result += var;
        first = false;
    }
    return first ? "0" : result;
}

std::string relation(char sense) {
    if (sense == 'L') return "<=";
    if (sense == 'G') return ">=";
    if (sense == 'E') return "=";
    throw std::runtime_error("Cannot print row with sense N as a constraint.");
}

void print_var_list_one_per_line(std::ostream& out, const std::vector<std::string>& vars) {
    for (const std::string& var : vars) out << var << '\n';
}

void write_constraint(std::ostream& out,
                      const Mps& m,
                      const std::vector<std::string>& all_order,
                      const std::string& row,
                      bool constraint_names) {
    out << ' ';
    if (constraint_names) out << row << ": ";

    const auto matrix_it = m.matrix.find(row);
    static const std::map<std::string, double> empty;
    const auto& coefs = (matrix_it == m.matrix.end()) ? empty : matrix_it->second;

    const auto rhs_it = m.rhs.find(row);
    const double rhs = (rhs_it == m.rhs.end()) ? 0.0 : rhs_it->second;

    out << linear_expr(coefs, all_order) << ' '
        << relation(m.rows.at(row).sense) << ' '
        << number(rhs) << '\n';
}

void write_qlp(const Mps& m,
               const Aux& aux,
               const std::string& out_path,
               bool maximize,
               bool constraint_names) {
    std::vector<std::string> upper_vars;
    std::vector<std::string> lower_vars;
    for (const auto& var : m.var_order) {
        if (aux.lower_var_set.count(var)) lower_vars.push_back(var);
        else upper_vars.push_back(var);
    }

    std::vector<std::string> all_order = upper_vars;
    all_order.insert(all_order.end(), lower_vars.begin(), lower_vars.end());

    std::ofstream out(out_path);
    if (!out) throw std::runtime_error("Cannot write QLP: " + out_path);

    out << (maximize ? "MAXIMIZE\n" : "MINIMIZE\n");

    std::map<std::string, double> obj;
    if (!m.obj_row.empty()) {
        const auto obj_it = m.matrix.find(m.obj_row);
        if (obj_it != m.matrix.end()) obj = obj_it->second;
    }
    // If the MPS contains no objective row, or an objective row without coefficients,
    // QLP still needs an objective section. We emit the constant zero objective.
    if (maximize) {
        for (auto& [_, coef] : obj) coef = -coef;
    }
    out << linear_expr(obj, all_order) << '\n';

    out << "SUBJECT TO\n";
    for (const std::string& row : m.row_order) {
        const auto& r = m.rows.at(row);
        if (r.sense == 'N' || aux.lower_row_set.count(row)) continue;
        write_constraint(out, m, all_order, row, constraint_names);
    }

    out << "UNCERTAINTY SUBJECT TO\n";
    for (const std::string& row : m.row_order) {
        const auto& r = m.rows.at(row);
        if (r.sense == 'N' || !aux.lower_row_set.count(row)) continue;
        write_constraint(out, m, all_order, row, constraint_names);
    }

    out << "BOUNDS\n";
    for (const std::string& var : all_order) {
        const Var& v = m.vars.at(var);
        out << number(v.lb) << " <= " << var << " <= " << number(v.ub) << '\n';
    }

    std::vector<std::string> binaries;
    std::vector<std::string> generals;
    for (const std::string& var : all_order) {
        const Var& v = m.vars.at(var);
        if (v.integer && std::fabs(v.lb) < 5e-13 && std::fabs(v.ub - 1.0) < 5e-13) {
            binaries.push_back(var);
        } else if (v.integer) {
            generals.push_back(var);
        }
    }

    if (!binaries.empty()) {
        out << "BINARIES\n";
        print_var_list_one_per_line(out, binaries);
    }
    if (!generals.empty()) {
        out << "GENERALS\n";
        print_var_list_one_per_line(out, generals);
    }

    out << "EXISTS\n";
    print_var_list_one_per_line(out, upper_vars);
    out << "ALL\n";
    print_var_list_one_per_line(out, lower_vars);
    out << "ORDER\n";
    print_var_list_one_per_line(out, all_order);
    out << "END\n";
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 4) {
            std::cerr << "Usage: " << argv[0]
                      << " instance.mps instance.aux output.qlp "
                      << "[--maximize] [--constraint-names]\n";
            return 2;
        }

        bool maximize = false;
        bool constraint_names = false;
        for (int i = 4; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--maximize") maximize = true;
            else if (arg == "--constraint-names") constraint_names = true;
            else throw std::runtime_error("Unknown option: " + arg);
        }

        const Mps mps = read_mps(argv[1]);
        const Aux aux = read_aux(argv[2]);
        write_qlp(mps, aux, argv[3], maximize, constraint_names);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
