#include "layout_config.hpp"

#include <fstream>
#include <sstream>
#include <vector>
#include "protocol.hpp"
#include <string>
#include <algorithm>

bool load_layout_params(const std::string& path, LayoutParams& out) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string tag;
    while (f >> tag) {
        if (tag == "S") {
            float a, b, c, d, e, ox, oy, ps = 1.0f, pox = 0.0f, poy = 0.0f, boy = 0.0f, loy = 0.0f;
            if (f >> a >> b >> c >> d >> e >> ox >> oy) {
                if (!(f >> ps >> pox >> poy >> boy >> loy)) {
                    // optional extra params may be missing
                }
                out.slotScale = a;
                out.symbolScale = b;
                out.playerScale = c;
                out.windowW = d;
                out.windowH = e;
                out.windowOffsetX = ox;
                out.windowOffsetY = oy;
                out.panelScale = ps;
                out.panelOffsetX = pox;
                out.panelOffsetY = poy;
                out.barOffsetY = boy;
                out.logOffsetY = loy;
                return true;
            }
        } else {
            // consume rest of line
            std::string rest;
            std::getline(f, rest);
        }
    }
    return false;
}

bool load_layout_file(const std::string& path, LayoutParams& out, std::vector<SlotLayout>& slots) {
    slots.clear();
    slots.resize(casino::MAX_PLAYERS);
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string tag;
    int idx = 0;
    while (f >> tag) {
        if (tag == "S") {
            float a, b, c, d, e, ox, oy, ps = 1.0f, pox = 0.0f, poy = 0.0f, boy = 0.0f, loy = 0.0f;
            if (f >> a >> b >> c >> d >> e >> ox >> oy) {
                if (!(f >> ps >> pox >> poy >> boy >> loy)) {
                    // optional extras may be missing
                }
                out.slotScale = a;
                out.symbolScale = b;
                out.playerScale = c;
                out.windowW = d;
                out.windowH = e;
                out.windowOffsetX = ox;
                out.windowOffsetY = oy;
                out.panelScale = ps;
                out.panelOffsetX = pox;
                out.panelOffsetY = poy;
                out.barOffsetY = boy;
                out.logOffsetY = loy;
            }
        } else if (tag == "P") {
            float x, y;
            float ss=out.slotScale, sy=out.symbolScale, ps=out.playerScale;
            float ww=out.windowW, wh=out.windowH, wx=out.windowOffsetX, wy=out.windowOffsetY;
            if (f >> x >> y) {
                // optional per-slot params
                if (!(f >> ss >> sy >> ps >> ww >> wh >> wx >> wy)) {
                    // keep defaults if not present
                }
                if (idx < (int)slots.size()) {
                    slots[idx].slotScale = ss;
                    slots[idx].symbolScale = sy;
                    slots[idx].playerScale = ps;
                    slots[idx].windowW = ww;
                    slots[idx].windowH = wh;
                    slots[idx].windowOffsetX = wx;
                    slots[idx].windowOffsetY = wy;
                    slots[idx].set = true;
                }
                idx++;
            }
        } else {
            std::string rest;
            std::getline(f, rest);
        }
    }
    return true;
}

static float extract_value(const std::string& s, const std::string& key, float def) {
    auto pos = s.find("\"" + key + "\"");
    if (pos == std::string::npos) return def;
    pos = s.find(':', pos);
    if (pos == std::string::npos) return def;
    pos++;
    // skip spaces
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\"')) pos++;
    size_t end = pos;
    while (end < s.size() && (isdigit(s[end]) || s[end]=='-' || s[end]=='+' || s[end]=='.' || s[end]=='e' || s[end]=='E')) end++;
    try { return std::stof(s.substr(pos, end-pos)); } catch(...) { return def; }
}

bool load_scene_file(const std::string& path, LayoutParams& params, std::vector<SlotLayout>& slots, std::vector<Vector2>* positions) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    slots.clear();
    slots.resize(casino::MAX_PLAYERS);
    if (positions) positions->clear();
    // parse ui
    params.panelScale = extract_value(content, "panelScale", params.panelScale);
    params.panelOffsetX = extract_value(content, "panelOffsetX", params.panelOffsetX);
    params.panelOffsetY = extract_value(content, "panelOffsetY", params.panelOffsetY);
    params.barOffsetY = extract_value(content, "barOffsetY", params.barOffsetY);
    params.logOffsetY = extract_value(content, "logOffsetY", params.logOffsetY);
    params.slotScale = extract_value(content, "slotScale", params.slotScale);
    params.symbolScale = extract_value(content, "symbolScale", params.symbolScale);
    params.playerScale = extract_value(content, "playerScale", params.playerScale);
    params.windowW = extract_value(content, "windowW", params.windowW);
    params.windowH = extract_value(content, "windowH", params.windowH);
    params.windowOffsetX = extract_value(content, "windowOffsetX", params.windowOffsetX);
    params.windowOffsetY = extract_value(content, "windowOffsetY", params.windowOffsetY);

    // parse slots array
    auto slotsPos = content.find("\"slots\"");
    if (slotsPos == std::string::npos) return true;
    auto arrStart = content.find('[', slotsPos);
    auto arrEnd = content.find(']', slotsPos);
    if (arrStart == std::string::npos || arrEnd == std::string::npos) return true;
    size_t pos = arrStart;
    if (positions) positions->resize(casino::MAX_PLAYERS, Vector2{0,0});
    int count = 0;
    while (true) {
        auto objStart = content.find('{', pos);
        if (objStart == std::string::npos || objStart > arrEnd) break;
        auto objEnd = content.find('}', objStart);
        if (objEnd == std::string::npos) break;
        std::string obj = content.substr(objStart, objEnd - objStart);
        SlotLayout sl;
        sl.slotScale = extract_value(obj, "slotScale", params.slotScale);
        sl.symbolScale = extract_value(obj, "symbolScale", params.symbolScale);
        sl.playerScale = extract_value(obj, "playerScale", params.playerScale);
        sl.windowW = extract_value(obj, "windowW", params.windowW);
        sl.windowH = extract_value(obj, "windowH", params.windowH);
        sl.windowOffsetX = extract_value(obj, "windowOffsetX", params.windowOffsetX);
        sl.windowOffsetY = extract_value(obj, "windowOffsetY", params.windowOffsetY);
        sl.set = true;
        float x = extract_value(obj, "x", 960.0f);
        float y = extract_value(obj, "y", 540.0f);
        int sid = (int)extract_value(obj, "id", (float)count);
        if (sid < 0 || sid >= casino::MAX_PLAYERS) sid = count;
        if (sid >= (int)slots.size()) slots.resize(sid + 1);
        slots[sid] = sl;
        if (positions) {
            if ((int)positions->size() <= sid) positions->resize(sid + 1);
            (*positions)[sid] = {x, y};
        }
        count++;
        pos = objEnd + 1;
    }
    return true;
}

static float find_prop_value(const std::string& segment, const std::string& prop, float def) {
    auto pos = segment.find("\"name\":\"" + prop + "\"");
    if (pos == std::string::npos) return def;
    pos = segment.find("\"value\":", pos);
    if (pos == std::string::npos) return def;
    pos += 8;
    size_t end = pos;
    while (end < segment.size() && (isdigit(segment[end]) || segment[end]=='-' || segment[end]=='+' || segment[end]=='.')) end++;
    try { return std::stof(segment.substr(pos, end - pos)); } catch(...) { return def; }
}

bool load_tiled_tmj(const std::string& path, LayoutParams& params, std::vector<SlotLayout>& slots, std::vector<Vector2>* positions) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    slots.clear();
    slots.resize(casino::MAX_PLAYERS);
    if (positions) { positions->clear(); positions->reserve(casino::MAX_PLAYERS); }

    // map-level properties (ui)
    params.panelScale = find_prop_value(content, "panelScale", params.panelScale);
    params.panelOffsetX = find_prop_value(content, "panelOffsetX", params.panelOffsetX);
    params.panelOffsetY = find_prop_value(content, "panelOffsetY", params.panelOffsetY);
    params.barOffsetY = find_prop_value(content, "barOffsetY", params.barOffsetY);
    params.logOffsetY = find_prop_value(content, "logOffsetY", params.logOffsetY);
    params.slotScale = find_prop_value(content, "slotScale", params.slotScale);
    params.symbolScale = find_prop_value(content, "symbolScale", params.symbolScale);
    params.playerScale = find_prop_value(content, "playerScale", params.playerScale);
    params.windowW = find_prop_value(content, "windowW", params.windowW);
    params.windowH = find_prop_value(content, "windowH", params.windowH);
    params.windowOffsetX = find_prop_value(content, "windowOffsetX", params.windowOffsetX);
    params.windowOffsetY = find_prop_value(content, "windowOffsetY", params.windowOffsetY);

    // parse objects with "type":"slot" or class containing slot
    size_t pos = 0;
    int idx = 0;
    while (true) {
        pos = content.find("\"type\":\"slot\"", pos);
        if (pos == std::string::npos) break;
        size_t objStart = content.rfind('{', pos);
        size_t objEnd = content.find('}', pos);
        if (objStart == std::string::npos || objEnd == std::string::npos) break;
        std::string obj = content.substr(objStart, objEnd - objStart);
        float x = extract_value(obj, "x", 960.0f);
        float y = extract_value(obj, "y", 540.0f);
        SlotLayout sl;
        sl.slotScale = find_prop_value(obj, "slotScale", params.slotScale);
        sl.symbolScale = find_prop_value(obj, "symbolScale", params.symbolScale);
        sl.playerScale = find_prop_value(obj, "playerScale", params.playerScale);
        sl.windowW = find_prop_value(obj, "windowW", params.windowW);
        sl.windowH = find_prop_value(obj, "windowH", params.windowH);
        sl.windowOffsetX = find_prop_value(obj, "windowOffsetX", params.windowOffsetX);
        sl.windowOffsetY = find_prop_value(obj, "windowOffsetY", params.windowOffsetY);
        sl.set = true;
        if (idx < (int)slots.size()) slots[idx] = sl;
        if (positions) positions->push_back({x, y});
        idx++;
        pos = objEnd + 1;
    }
    return true;
}
