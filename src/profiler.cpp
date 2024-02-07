#include "pocketpy/profiler.h"

namespace pkpy{

static std::string left_pad(std::string s, int width){
    int n = width - s.size();
    if(n <= 0) return s;
    return std::string(n, ' ') + s;
}

static std::string to_string_1f(f64 x){
    char buf[32];
    snprintf(buf, 32, "%.1f", x);
    return buf;
}

std::string_view LineRecord::line_content() const {
    auto [_0, _1] = src->_get_line(line);
    return std::string_view(_0, _1-_0);
}

void LineProfiler::begin(){
    prev_time = 0;
    prev_record = nullptr;
    prev_line = -1;
    records.clear();
}

void LineProfiler::_step(Frame *frame){
    std::string_view filename = frame->co->src->filename.sv();
    int line = frame->co->lines[frame->_ip];
    // std::string_view function = frame->co->name.sv();

    if(prev_record == nullptr){
        prev_time = clock();
    }else{
        _step_end();
    }

    std::map<int, LineRecord>& file_records = records[filename];
    auto [it, ok] = file_records.insert({line, LineRecord(line, frame->co->src.get())});
    prev_record = &(it->second);
}

void LineProfiler::_step_end(){
    clock_t now = clock();
    clock_t delta = now - prev_time;
    prev_time = now;
    if(prev_record->line != prev_line){
        prev_record->hits++;
        prev_line = prev_record->line;
    }
    prev_record->time += delta;
}

void LineProfiler::end(){
    _step_end();
}

Str LineProfiler::stats(){
    SStream ss;
    for(auto& [filename, file_records] : records){
        clock_t total_time = 0;
        for(auto& [line, record] : file_records){
            total_time += record.time;
        }
        ss << "Total time: " << (f64)total_time / CLOCKS_PER_SEC << "s\n";
        ss << "File: " << filename << "\n";
        // ss << "Function: " << "<?>" << "at line " << -1 << "\n";
        ss << "Line #      Hits         Time  Per Hit   % Time  Line Contents\n";
        ss << "==============================================================\n";
        for(auto& [line, record]: file_records){
            ss << left_pad(std::to_string(line), 6);
            ss << left_pad(std::to_string(record.hits), 10);
            ss << left_pad(std::to_string(record.time), 13);
            ss << left_pad(std::to_string(record.time / record.hits), 9);
            ss << left_pad(to_string_1f(record.time * (f64)100 / total_time), 9);
            ss << "  " << record.line_content() << "\n";
        }
        ss << "\n";
    }
    return ss.str();
}

}   // namespace pkpy