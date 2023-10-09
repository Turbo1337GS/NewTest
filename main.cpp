#include "GigaWeb.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>
#include <iostream>
#include <unordered_set>
#include <fstream>
#include <chrono>
#include <thread>

// https://github.com/Turbo1337GS/GigaWeb
// g++ -std=c++20 ./*.cpp ./*.hpp -lcurl -lgumbo -lboost_system -o GigaSoft && ./GigaSoft

// Global variables for web object and visited URLs and Texts
GigaWeb *gw = new GigaWeb();
std::unordered_set<std::string> visitedUrls;
std::unordered_set<std::string> visitedTexts;

// Function to generate MD5 hash for a given string
std::string generateMD5(const std::string &input) {
    // Initialize MD5 hash
    boost::uuids::detail::md5 hash;
    boost::uuids::detail::md5::digest_type digest;

    // Process bytes from the input string
    hash.process_bytes(input.data(), input.size());

    // Obtain the digest
    hash.get_digest(digest);

    // Convert digest to string
    const auto charDigest = reinterpret_cast<const char *>(&digest);
    std::string result;
    boost::algorithm::hex(charDigest, charDigest + sizeof(boost::uuids::detail::md5::digest_type), std::back_inserter(result));
    return result;
}

// Function to create a file if it doesn't exist
void createFileIfNotExists(const std::string &filename) {
    std::ifstream infile(filename);
    if (!infile.good()) {
        std::ofstream outfile(filename);
        if (outfile.is_open()) {
            outfile.close();
            std::cout << "File " << filename << " has been created." << std::endl;
        } else {
            std::cerr << "Cannot create file: " << filename << std::endl;
        }
    } else {
        std::cout << "File " << filename << " already exists." << std::endl;
    }
}

// Function to get a value from an INI file
std::string get_ini_value(const std::string &tree_name) {
    boost::property_tree::ptree pt;
    try {
        boost::property_tree::read_ini("ini.cfg", pt);
    } catch (const boost::property_tree::ini_parser::ini_parser_error &e) {
        std::cout << "Error reading file: ini.cfg  " << e.message() << std::endl;
        return "";
    }

    // Retrieve the value based on tree_name
    std::string value;
    try {
        value = pt.get<std::string>(tree_name);
    } catch (const boost::property_tree::ptree_bad_path &e) {
        std::cout << "Key not found: " << e.what() << std::endl;
        return "";
    }
    return value;
}

// Recursive function to fetch URLs and extract text content
void recursiveFetch(const std::string &INITurl, const std::string &FileName, int sleepTime = 0) {
    // If the URL is already visited, exit the function
    if (visitedUrls.find(INITurl) != visitedUrls.end()) {
        return;
    }
    visitedUrls.insert(INITurl);

    std::string html;
    bool fetched = gw->fetchWebContent(INITurl, html);
    if (!fetched) {
        std::cerr << "Error fetching: " << INITurl << std::endl;
        return;
    }

    // Extract URLs from HTML content
    auto links = gw->extractURLs(html);
    for (const auto &URL : links) {
        if (strstr(URL.c_str(), "https://pythoninsider.blogspot.com/") ||
            strstr(URL.c_str(), "https://docs.python.org/") ||
            strstr(URL.c_str(), "https://python.org/")) {

            std::string html = "";
            bool fetched = gw->fetchWebContent(URL, html);
            if (fetched) {
                // Extract main content
                auto mainContent = gw->getMultipleContents(html);
                for (const auto &content : mainContent) {
                    std::string contentMD5 = generateMD5(content);
                    if (visitedTexts.find(contentMD5) == visitedTexts.end()) {
                        visitedTexts.insert(contentMD5);
                        std::fstream a(FileName, std::ios::app);
                        a << content << "\n";
                    }
                }
            }
            // Recursive call to fetch next URLs
            std::this_thread::sleep_for(std::chrono::seconds(sleepTime)); // Use this instead of usleep
            recursiveFetch(URL, FileName);
            
        }
    }
}

// Main function
int main() {
    // Read values from INI file
    std::string INITurl = get_ini_value("startUrl");
    std::string FileName = get_ini_value("fileName");
    std::string sleepTime = get_ini_value("sleepTime");

    createFileIfNotExists(FileName.c_str());

    if (INITurl.empty()) {
        std::cerr << "INITurl value not found in ini.cfg." << std::endl;
        return 1;
    }

    // Start the recursive fetching process
    recursiveFetch(INITurl, FileName, atoi(sleepTime.c_str()));

    // Clean up
    delete gw;
    return 0;
}
