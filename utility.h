//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// stringicmp
//
// provides case insensitive string comparison for std::map
//
struct stringicmp
{
    bool operator()(const std::string& s1, const std::string& s2) const
    {
        return (lstrcmpi(s1.c_str(), s2.c_str()) < 0);
    }
};