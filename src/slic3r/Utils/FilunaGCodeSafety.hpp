// Filuna / Orca C++17. AGPL-3.0-or-later.
// Read-only OEM travel geometry preflight. Never modifies G-code.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace Slic3r { namespace Filuna { namespace Safety {

struct Risk {
    size_t line = 0;
    int layer = -1;
    std::array<double, 3> xyz {{0, 0, 0}};
    double printed_z = 0;
    const char* reason = "TRAVEL_CROSSES_PRINTED_GEOMETRY";
};

struct Report {
    enum class Decision { Pass, Warn, Block, Review };
    Decision decision = Decision::Review;
    std::vector<Risk> collisions;
    std::vector<Risk> warnings;
    size_t object_extrusions = 0;
    size_t tower_extrusions = 0;
    size_t checked_travels = 0;
    std::string explanation;
};

struct Policy {
    std::array<double, 3> max_xyz {{270, 270, 270}};
    double pitch = 0.5;
    double clearance = 0.2;
    bool bounds_known = true;
};

inline std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });
    return value;
}

inline bool has(const std::string& value, const char* part)
{
    return value.find(part) != std::string::npos;
}

inline std::unordered_map<char, double> words(const std::string& code)
{
    std::unordered_map<char, double> out;
    for (size_t i = 0; i < code.size(); ++i) {
        const char axis = char(std::toupper(static_cast<unsigned char>(code[i])));
        if (axis != 'X' && axis != 'Y' && axis != 'Z' &&
            axis != 'E' && axis != 'I' && axis != 'J')
            continue;
        const char* start = code.c_str() + i + 1;
        while (*start == ' ' || *start == '\t')
            ++start;
        char* end = nullptr;
        const double value = std::strtod(start, &end);
        if (end > start && std::isfinite(value)) {
            out[axis] = value;
            i = size_t(end - code.c_str() - 1);
        }
    }
    return out;
}

inline int64_t cell_key(int32_t x, int32_t y)
{
    return int64_t((uint64_t(uint32_t(x)) << 32) | uint32_t(y));
}

struct Deposit {
    double z = 0;
    size_t line = 0;
};

inline Report inspect(const std::string& gcode, const Policy& policy = {})
{
    Report report;
    if (gcode.empty() || gcode.size() > 50u * 1024u * 1024u) {
        report.explanation = "GCODE_EMPTY_OR_TOO_LARGE";
        return report;
    }
    if (!(policy.pitch >= 0.1 && policy.pitch <= 2.0 &&
          policy.clearance >= 0 && policy.clearance <= 3.0)) {
        report.explanation = "INVALID_POLICY";
        return report;
    }

    std::vector<std::string> lines;
    {
        std::istringstream reader(gcode);
        std::string line;
        while (std::getline(reader, line))
            lines.emplace_back(std::move(line));
    }
    size_t final_object_close = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        const auto sep = lines[i].find(';');
        if (sep != std::string::npos &&
            (has(lower(lines[i].substr(sep + 1)), "stop printing object") ||
             has(lower(lines[i].substr(sep + 1)), "exclude_object_end")))
            final_object_close = i + 1;
    }

    std::unordered_map<int64_t, Deposit> occupancy;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    std::array<double, 3> pos {{nan, nan, nan}};
    double epos = 0;
    bool relative_xyz = false, relative_e = false;
    bool prior_object = false;
    bool final_tail = false, in_wipe = false, tower = false;
    size_t recent_tool = 0, final_marker = 0, spiral_marker = 0;
    int layer = -1;

    auto walk = [&](const std::array<double,3>& a,
                    const std::array<double,3>& b,
                    const std::unordered_map<char,double>& args,
                    bool clockwise, bool arc) {
        std::vector<std::array<double,3>> points;
        const double dx = b[0] - a[0], dy = b[1] - a[1];
        if (arc && (args.count('I') || args.count('J'))) {
            const double cx = a[0] + (args.count('I') ? args.at('I') : 0);
            const double cy = a[1] + (args.count('J') ? args.at('J') : 0);
            const double r = std::hypot(a[0] - cx, a[1] - cy);
            if (r < 0.0001 || !std::isfinite(r))
                return points;
            const double start = std::atan2(a[1] - cy, a[0] - cx);
            const double end = std::atan2(b[1] - cy, b[0] - cx);
            const double twopi = 6.2831853071795864769;
            double delta = std::fmod(end - start + twopi, twopi);
            if (clockwise)
                delta = -std::fmod(start - end + twopi, twopi);
            if (std::abs(delta) < 1e-8)
                delta = clockwise ? -twopi : twopi;
            const size_t n = std::min(size_t(20000),
                std::max(size_t(4), size_t(std::ceil(std::abs(delta)*r/policy.pitch))));
            points.reserve(n + 1);
            for (size_t j = 0; j <= n; ++j) {
                const double t = double(j) / n;
                const double theta = start + delta * t;
                points.push_back({{cx + r*std::cos(theta),
                                   cy + r*std::sin(theta),
                                   a[2] + (b[2] - a[2])*t}});
            }
            return points;
        }
        if (arc)
            return points;
        const size_t n = std::min(size_t(20000),
            std::max(size_t(1), size_t(std::ceil(std::hypot(dx,dy)/policy.pitch))));
        points.reserve(n + 1);
        for (size_t j = 0; j <= n; ++j) {
            const double t = double(j) / n;
            points.push_back({{a[0] + dx*t, a[1] + dy*t,
                               a[2] + (b[2] - a[2])*t}});
        }
        return points;
    };

    for (size_t k = 0; k < lines.size(); ++k) {
        const size_t line_no = k + 1;
        const auto sep = lines[k].find(';');
        const std::string comment = sep == std::string::npos ?
            "" : lower(lines[k].substr(sep + 1));
        const std::string code = lines[k].substr(0, sep);
        if (has(comment, "type:prime tower") || has(comment, "type:wipe tower"))
            tower = true;
        else if ((has(comment, "type:") && !has(comment, "prime tower") &&
                  !has(comment, "wipe tower")) || has(comment, "exclude_object_start"))
            tower = false;
        if (has(comment, "wipe_start")) in_wipe = true;
        if (has(comment, "wipe_end")) in_wipe = false;
        if (line_no == final_object_close) final_tail = true;
        if (has(comment, "final_purge") || has(comment, "final purge") ||
            has(comment, "final wipe"))
            final_marker = line_no;
        if (has(comment, "spiral") && (has(comment, "hop") || has(comment,"lift")))
            spiral_marker = line_no;
        // Only the exact Orca LAYER_CHANGE marker advances the layer.
        // BEFORE_LAYER_CHANGE and AFTER_LAYER_CHANGE are not new layers.
        const auto first_text = comment.find_first_not_of(" \t");
        if (first_text != std::string::npos &&
            comment.compare(first_text, 12, "layer_change") == 0)
            ++layer;
        const auto layerpos = comment.find("layer:");
        if (layerpos != std::string::npos) {
            char* last = nullptr;
            const char* start = comment.c_str() + layerpos + 6;
            const long parsed = std::strtol(start, &last, 10);
            if (last > start && parsed >= 0 && parsed < 1000000)
                layer = int(parsed);
        }

        std::istringstream cs(code);
        std::string op;
        cs >> op;
        op = lower(op);
        if (op.empty()) continue;
        if (op == "g90") { relative_xyz = false; continue; }
        if (op == "g91") { relative_xyz = true; continue; }
        if (op == "m82") { relative_e = false; continue; }
        if (op == "m83") { relative_e = true; continue; }
        if (op[0] == 't' && op.size() > 1 &&
            std::all_of(op.begin()+1, op.end(), [](unsigned char c){return std::isdigit(c);})) {
            if (prior_object) recent_tool = line_no;
            continue;
        }
        const auto args = words(code);
        if (op == "g28") {
            // A relative move before a trustworthy coordinate anchor is unknown.
            bool any = false;
            for (size_t i = 0; i < 3; ++i) {
                const char axis = "XYZ"[i];
                if (args.count(axis)) { pos[i] = nan; any = true; }
            }
            if (!any) pos = {{nan, nan, nan}};
            continue;
        }
        if (op == "g92") {
            for (size_t i = 0; i < 3; ++i)
                if (args.count("XYZ"[i])) pos[i] = args.at("XYZ"[i]);
            if (args.count('E')) epos = args.at('E');
            continue;
        }
        if (op != "g0" && op != "g1" && op != "g2" && op != "g3")
            continue;
        const std::array<double,3> old = pos;
        for (size_t i = 0; i < 3; ++i) {
            const char axis = "XYZ"[i];
            if (args.count(axis))
                pos[i] = relative_xyz ?
                    (std::isfinite(pos[i]) ? pos[i] + args.at(axis) : nan) :
                    args.at(axis);
        }
        double delta_e = 0;
        if (args.count('E')) {
            const double value = args.at('E');
            const double next = relative_e ? epos + value : value;
            delta_e = next - epos;
            epos = next;
        }
        if (!std::all_of(pos.begin(),pos.end(),[](double v){return std::isfinite(v);}) ||
            !std::all_of(old.begin(),old.end(),[](double v){return std::isfinite(v);}))
            continue;
        const bool arc = op == "g2" || op == "g3";
        const auto points = walk(old,pos,args,op == "g2",arc);
        if (points.empty()) {
            if (arc) { report.explanation = "UNRESOLVED_GCODE_ARC"; }
            continue;
        }
        const bool spiral = ((spiral_marker != 0) && line_no >= spiral_marker &&
                             (line_no - spiral_marker <= 12)) ||
                            (arc && pos[2] > old[2] + 1e-5);
        if (policy.bounds_known) {
            for (const auto& q : points) {
                bool out = false;
                for (size_t i = 0; i < 3; ++i)
                    if (q[i] < -0.001 || q[i] > policy.max_xyz[i] + 0.001)
                        out = true;
                if (out) {
                    if (report.warnings.size() < 32) {
                        Risk event;
                        event.line = line_no; event.layer = layer; event.xyz = q;
                        event.reason = spiral ? "AUTO_SPIRAL_OUT_OF_VOLUME" : "TRAVEL_OUT_OF_PRINT_VOLUME";
                        report.warnings.emplace_back(event);
                    }
                    break;
                }
            }
        }
        const double xy = std::hypot(pos[0]-old[0], pos[1]-old[1]);
        if (xy < 1e-4) continue;
        if (delta_e > 1e-5) {
            prior_object = true;
            if (tower) { ++report.tower_extrusions; continue; }
            ++report.object_extrusions;
            for (const auto& q : points) {
                const int32_t x = int32_t(std::floor(q[0]/policy.pitch));
                const int32_t y = int32_t(std::floor(q[1]/policy.pitch));
                const int32_t xp = int32_t(std::floor((q[0]+policy.pitch/2)/policy.pitch));
                const int32_t xm = int32_t(std::floor((q[0]-policy.pitch/2)/policy.pitch));
                const int32_t yp = int32_t(std::floor((q[1]+policy.pitch/2)/policy.pitch));
                const int32_t ym = int32_t(std::floor((q[1]-policy.pitch/2)/policy.pitch));
                const std::array<int64_t,5> keys {{
                    cell_key(x,y),cell_key(xp,y),cell_key(xm,y),
                    cell_key(x,yp),cell_key(x,ym)
                }};
                for (auto key : keys) {
                    auto it = occupancy.find(key);
                    if (it == occupancy.end() || it->second.z <= q[2])
                        occupancy[key] = Deposit{q[2],line_no};
                }
            }
            continue;
        }
        const bool critical = final_tail || (final_marker && line_no >= final_marker &&
                    line_no - final_marker <= 25) ||
                    (recent_tool && line_no >= recent_tool && line_no - recent_tool <= 20);
        if (!critical || !prior_object || (in_wipe && delta_e < 0 && xy <= 5))
            continue;
        ++report.checked_travels;
        for (size_t j = 1; j + 1 < points.size(); ++j) {
            const double fraction = double(j) / double(points.size()-1);
            if (fraction < 0.04 || fraction > 0.96) continue;
            const auto& q = points[j];
            const int32_t ix = int32_t(std::floor(q[0]/policy.pitch));
            const int32_t iy = int32_t(std::floor(q[1]/policy.pitch));
            const auto it = occupancy.find(cell_key(ix,iy));
            if (it != occupancy.end() && it->second.line + 3 < line_no &&
                q[2] <= it->second.z + policy.clearance) {
                if (report.collisions.size() < 32) {
                    Risk event;
                    event.line = line_no; event.layer = layer; event.xyz = q;
                    event.printed_z = it->second.z;
                    report.collisions.emplace_back(event);
                }
                break;
            }
        }
    }
    if (!report.collisions.empty()) {
        report.decision = Report::Decision::Block;
        report.explanation = "GCODE_TRAVEL_CROSSES_ALREADY_PRINTED_PART";
    } else if (!report.warnings.empty()) {
        report.decision = Report::Decision::Warn;
        report.explanation = "GCODE_MOTION_OUTSIDE_DECLARED_PRINT_VOLUME";
    } else if (!report.explanation.empty() || report.object_extrusions == 0) {
        report.decision = Report::Decision::Review;
        if (report.explanation.empty()) report.explanation = "NO_PRINTED_OBJECT_GEOMETRY";
    } else {
        report.decision = Report::Decision::Pass;
        report.explanation = "NO_DETECTED_RISK_WITHIN_SCOPED_ANALYSIS";
    }
    return report;
}

inline Report inspect_file(const std::string& path, const Policy& policy = {})
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        Report missing;
        missing.explanation = "GCODE_FILE_NOT_READABLE";
        return missing;
    }
    std::ostringstream text;
    text << in.rdbuf();
    if (!in.good() && !in.eof()) {
        Report invalid;
        invalid.explanation = "GCODE_FILE_READ_ERROR";
        return invalid;
    }
    return inspect(text.str(),policy);
}

}}} // namespace Slic3r::Filuna::Safety
