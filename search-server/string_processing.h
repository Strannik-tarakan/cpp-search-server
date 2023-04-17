#pragma once


std::string ReadLine();

int ReadLineWithNumber();

template <typename type>
std::ostream& operator<<(std::ostream& out, const std::vector<type>& container) {
    out << "[";
    bool first = true;
    for (const type& element : container) {
        if (!first) {

            out << ", " << element;
        }
        else {
            out << element;
            first = false;
        }

    }
    out << "]";
    return out;
}

template <typename type>
std::ostream& operator<<(std::ostream& out, const std::set<type>& container) {
    out << "{";
    bool first = true;
    for (const type& element : container) {
        if (!first) {

            out << ", " << element;
        }
        else {
            out << element;
            first = false;
        }

    }
    out << "}";
    return out;
}

template <typename key, typename value>
std::ostream& operator<<(std::ostream& out, const std::map<key, value>& container) {
    out << "{";
    bool first = true;
    for (const auto& [key_, value_] : container) {
        if (!first) {

            out << ", " << key_ << ": " << value_;
        }
        else {
            out << key_ << ": " << value_;
            first = false;
        }

    }
    out << "}";
    return out;
}



