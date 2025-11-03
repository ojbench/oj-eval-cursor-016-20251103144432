#include <bits/stdc++.h>
using namespace std;

// Persistent KV store: key -> sorted unique vector<int>
// Loads from binary file kvstore.bin if present, saves back on exit.
// Binary format:
// [u32 keyCount]
// Repeated keyCount times:
//   [u16 keyLen][key bytes][u32 valueCount][valueCount * i32 values sorted asc]

static const char *DATA_FILE = "kvstore.bin";

struct Hasher {
    size_t operator()(const string &s) const noexcept {
        return std::hash<string>{}(s);
    }
};

static inline bool read_exact(ifstream &ifs, char *buf, size_t n) {
    if (!n) return true;
    ifs.read(buf, static_cast<streamsize>(n));
    return static_cast<size_t>(ifs.gcount()) == n;
}

static inline bool write_exact(ofstream &ofs, const char *buf, size_t n) {
    if (!n) return true;
    ofs.write(buf, static_cast<streamsize>(n));
    return ofs.good();
}

static inline uint32_t read_u32(ifstream &ifs) {
    uint32_t v;
    ifs.read(reinterpret_cast<char*>(&v), sizeof(v));
    return v;
}

static inline uint16_t read_u16(ifstream &ifs) {
    uint16_t v;
    ifs.read(reinterpret_cast<char*>(&v), sizeof(v));
    return v;
}

static inline int32_t read_i32(ifstream &ifs) {
    int32_t v;
    ifs.read(reinterpret_cast<char*>(&v), sizeof(v));
    return v;
}

static inline void write_u32(ofstream &ofs, uint32_t v) {
    ofs.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

static inline void write_u16(ofstream &ofs, uint16_t v) {
    ofs.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

static inline void write_i32(ofstream &ofs, int32_t v) {
    ofs.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    unordered_map<string, vector<int>, Hasher> store;
    store.reserve(262144);

    // Load persisted data if exists
    {
        ifstream ifs(DATA_FILE, ios::binary);
        if (ifs.good()) {
            uint32_t keyCount = 0;
            ifs.read(reinterpret_cast<char*>(&keyCount), sizeof(keyCount));
            if (ifs.good()) {
                for (uint32_t i = 0; i < keyCount; ++i) {
                    uint16_t klen = read_u16(ifs);
                    if (!ifs.good()) { break; }
                    string key;
                    key.resize(klen);
                    if (!read_exact(ifs, key.data(), klen)) { break; }
                    uint32_t vcnt = read_u32(ifs);
                    if (!ifs.good()) { break; }
                    vector<int> vals;
                    vals.resize(vcnt);
                    if (vcnt) {
                        ifs.read(reinterpret_cast<char*>(vals.data()), static_cast<streamsize>(vcnt * sizeof(int32_t)));
                        if (!ifs.good()) { break; }
                    }
                    store.emplace(std::move(key), std::move(vals));
                }
            }
        }
    }

    int n;
    if (!(cin >> n)) return 0;
    string op;
    op.reserve(8);
    string key;
    key.reserve(64);
    for (int i = 0; i < n; ++i) {
        cin >> op;
        if (op[0] == 'i') { // insert
            int val;
            cin >> key >> val;
            auto &vec = store[key];
            // Insert val in sorted order if not exists
            auto it = lower_bound(vec.begin(), vec.end(), val);
            if (it == vec.end() || *it != val) {
                vec.insert(it, val);
            }
        } else if (op[0] == 'd') { // delete
            int val;
            cin >> key >> val;
            auto itKey = store.find(key);
            if (itKey != store.end()) {
                auto &vec = itKey->second;
                auto it = lower_bound(vec.begin(), vec.end(), val);
                if (it != vec.end() && *it == val) {
                    vec.erase(it);
                    if (vec.empty()) {
                        store.erase(itKey);
                    }
                }
            }
        } else { // find
            cin >> key;
            auto itKey = store.find(key);
            if (itKey == store.end() || itKey->second.empty()) {
                cout << "null\n";
            } else {
                const auto &vec = itKey->second;
                for (size_t j = 0; j < vec.size(); ++j) {
                    if (j) cout << ' ';
                    cout << vec[j];
                }
                cout << '\n';
            }
        }
    }

    // Save persisted data
    {
        ofstream ofs(DATA_FILE, ios::binary | ios::trunc);
        uint32_t keyCount = static_cast<uint32_t>(store.size());
        write_u32(ofs, keyCount);
        if (keyCount) {
            // To reduce randomness, iterate in deterministic order to help compression/caching if any
            vector<pair<string, vector<int>*>> entries;
            entries.reserve(store.size());
            for (auto &kv : store) entries.emplace_back(kv.first, &kv.second);
            sort(entries.begin(), entries.end(), [](const auto &a, const auto &b){ return a.first < b.first; });
            for (const auto &kv : entries) {
                const string &k = kv.first;
                const vector<int> &vals = *kv.second;
                write_u16(ofs, static_cast<uint16_t>(k.size()));
                write_exact(ofs, k.data(), k.size());
                write_u32(ofs, static_cast<uint32_t>(vals.size()));
                if (!vals.empty()) {
                    ofs.write(reinterpret_cast<const char*>(vals.data()), static_cast<streamsize>(vals.size() * sizeof(int32_t)));
                }
            }
        }
        ofs.flush();
    }

    return 0;
}
