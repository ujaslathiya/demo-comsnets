#pragma once
#include <vector>

namespace FiCo4OMNeT {

class CanBitStuffing {
public:
    static std::vector<bool> stuff(const std::vector<bool>& in) {
        std::vector<bool> out;
        bool last = in[0];
        int count = 1;
        out.push_back(last);

        for (size_t i = 1; i < in.size(); ++i) {
            bool b = in[i];
            if (b == last) count++;
            else count = 1;

            if (count == 5) {
                out.push_back(!b);   // stuff bit
                last = !b;
                count = 1;
            }
            out.push_back(b);
            last = b;
        }
        return out;
    }

    static bool destuff(const std::vector<bool>& in, std::vector<bool>& out) {
        bool last = in[0];
        int count = 1;
        out.push_back(last);

        for (size_t i = 1; i < in.size(); ++i) {
            bool b = in[i];
            if (b == last) count++;
            else count = 1;

            if (count == 6) return false; // stuff error

            if (count == 5) {
                i++;
                if (i >= in.size() || in[i] == last) return false;
                count = 1;
                last = in[i];
                out.push_back(last);
                continue;
            }
            out.push_back(b);
            last = b;
        }
        return true;
    }
};

}

