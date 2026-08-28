#define D2T_TESTING
#include "../diff2test.cpp"
#include <fstream>
#include <iostream>
namespace {
int failures=0;
void check(bool c,const char*n){if(!c){std::cerr<<"FAIL: "<<n<<'\n';++failures;}}
void put(const std::filesystem::path&p,std::string_view t){std::filesystem::create_directories(p.parent_path());std::ofstream o(p);o<<t;}
d2t::cli::AnalyzeOptions opts(const std::filesystem::path& root){
    d2t::cli::AnalyzeOptions o;
    o.project_root=root/"project";o.build_root=root/"build";o.changed_files=(root/"changed.txt").string();
    o.cmake_reply=root/"build/.cmake/api/v1/reply";o.ctest_info=root/"build/ctest-info.json";o.dep_list=root/"build/deps.txt";o.format="names";
    return o;
}
void fixture(const std::filesystem::path& root){
    auto p=root/"project";auto b=root/"build";auto r=b/".cmake/api/v1/reply";
    std::filesystem::create_directories(p/"src");std::filesystem::create_directories(p/"tests");std::filesystem::create_directories(p/"include");std::filesystem::create_directories(r);
    put(r/"index-a.json",R"({"reply":{"codemodel-v2":{"jsonFile":"codemodel.json","kind":"codemodel","version":{"major":2}}}})");
    put(r/"codemodel.json",std::string("{\"paths\":{\"source\":\"")+p.string()+"\",\"build\":\""+b.string()+"\"},\"configurations\":[{\"name\":\"\",\"targets\":[{\"id\":\"alpha::x\",\"jsonFile\":\"target-alpha.json\"},{\"id\":\"alpha_test::x\",\"jsonFile\":\"target-alpha-test.json\"}]}]}");
    put(r/"target-alpha.json",R"({"id":"alpha::x","name":"alpha","type":"STATIC_LIBRARY","sources":[{"path":"src/alpha.cpp"}],"artifacts":[],"dependencies":[]})");
    put(r/"target-alpha-test.json",R"({"id":"alpha_test::x","name":"alpha_test","type":"EXECUTABLE","sources":[{"path":"tests/alpha_test.cpp"}],"artifacts":[{"path":"alpha_test"}],"dependencies":[{"id":"alpha::x"}]})");
    put(b/"ctest-info.json",std::string("{\"kind\":\"ctestInfo\",\"version\":{\"major\":1},\"tests\":[{\"name\":\"AlphaTest\",\"command\":[\"")+(b/"alpha_test").string()+"\"]}]}");
    put(b/"CMakeFiles/alpha.dir/src/alpha.cpp.o.d",std::string("CMakeFiles/alpha.dir/src/alpha.cpp.o: ")+(p/"src/alpha.cpp").string()+" "+(p/"include/alpha.hpp").string()+"\n");
    put(b/"CMakeFiles/alpha_test.dir/tests/alpha_test.cpp.o.d",std::string("CMakeFiles/alpha_test.dir/tests/alpha_test.cpp.o: ")+(p/"tests/alpha_test.cpp").string()+" "+(p/"include/alpha.hpp").string()+"\n");
    put(b/"deps.txt","CMakeFiles/alpha.dir/src/alpha.cpp.o.d\nCMakeFiles/alpha_test.dir/tests/alpha_test.cpp.o.d\n");
    put(root/"changed.txt","include/alpha.hpp\n");
}
}
int main(){
    auto root=std::filesystem::temp_directory_path()/"d2t-impact-tests";std::filesystem::remove_all(root);fixture(root);
    auto o=opts(root);auto good=d2t::analysis::analyze(o);check(good.outcome==d2t::core::Outcome::SubsetSelected,"subset status");check(good.selected_tests==std::vector<std::string>{"AlphaTest"},"subset contents");
    put(root/"build/deps.txt","CMakeFiles/alpha_test.dir/tests/alpha_test.cpp.o.d\n");auto missing=d2t::analysis::analyze(o);check(missing.outcome==d2t::core::Outcome::FullSuiteSelected,"missing dep fallback");check(missing.selected_tests==std::vector<std::string>{"AlphaTest"},"fallback emits full catalogue");
    fixture(root);put(root/"changed.txt","docs/unknown.md\n");auto unknown=d2t::analysis::analyze(o);check(unknown.outcome==d2t::core::Outcome::FullSuiteSelected,"unknown change fallback");
    fixture(root);std::filesystem::remove(root/"build/ctest-info.json");auto nocat=d2t::analysis::analyze(o);check(nocat.outcome==d2t::core::Outcome::FullSuiteRequired,"missing catalogue required");check(nocat.selected_tests.empty(),"missing catalogue emits no names");
    std::filesystem::remove_all(root);if(failures==0){std::cout<<"impact_tests: all checks passed\n";return 0;}return 1;
}
