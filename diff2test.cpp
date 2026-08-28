#include <algorithm>
#include <charconv>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace d2t::core {
enum class Outcome { SubsetSelected=0, FullSuiteSelected=10, FullSuiteRequired=11, UsageError=64, InputError=65, InternalError=70 };
constexpr std::string_view kVersion = "0.1.0-dev";
int exit_code(Outcome o){ return static_cast<int>(o); }
}

namespace d2t::json {
struct Position { std::size_t offset=0,line=1,column=1; };
struct Error { Position position; std::string code; std::string message; };
struct Null final {};
struct Value;
using Array=std::vector<Value>;
using Object=std::map<std::string,Value,std::less<>>;
struct Value {
    using Storage=std::variant<Null,bool,double,std::string,Array,Object>;
    Storage data;
    Value():data(Null{}){}
    explicit Value(Null v):data(v){} explicit Value(bool v):data(v){} explicit Value(double v):data(v){}
    explicit Value(std::string v):data(std::move(v)){} explicit Value(Array v):data(std::move(v)){} explicit Value(Object v):data(std::move(v)){}
    bool is_null()const{return std::holds_alternative<Null>(data);} bool is_bool()const{return std::holds_alternative<bool>(data);}
    bool is_number()const{return std::holds_alternative<double>(data);} bool is_string()const{return std::holds_alternative<std::string>(data);}
    bool is_array()const{return std::holds_alternative<Array>(data);} bool is_object()const{return std::holds_alternative<Object>(data);}
    const bool& as_bool()const{return std::get<bool>(data);} const double& as_number()const{return std::get<double>(data);}
    const std::string& as_string()const{return std::get<std::string>(data);} const Array& as_array()const{return std::get<Array>(data);}
    const Object& as_object()const{return std::get<Object>(data);}
};
struct ParseOptions { std::size_t max_bytes=64U*1024U*1024U,max_nesting=256U,max_string_bytes=16U*1024U*1024U; };
struct ParseResult { std::optional<Value> value; std::optional<Error> error; bool ok()const{return value.has_value();} };

namespace detail {
bool cont(unsigned char c){return (c&0xC0U)==0x80U;}
bool valid_utf8(std::string_view s){
    for(std::size_t i=0;i<s.size();){
        unsigned char c=static_cast<unsigned char>(s[i]); if(c<=0x7F){++i;continue;}
        std::size_t n=0; std::uint32_t cp=0;
        if((c&0xE0U)==0xC0U){ if(c<0xC2U)return false;n=2;cp=c&0x1FU;}
        else if((c&0xF0U)==0xE0U){n=3;cp=c&0x0FU;}
        else if((c&0xF8U)==0xF0U){if(c>0xF4U)return false;n=4;cp=c&0x07U;}
        else return false;
        if(i+n>s.size())return false;
        for(std::size_t j=1;j<n;++j){unsigned char d=static_cast<unsigned char>(s[i+j]);if(!cont(d))return false;cp=(cp<<6U)|(d&0x3FU);}
        if((n==3&&cp<0x800U)||(n==4&&cp<0x10000U)||(cp>=0xD800U&&cp<=0xDFFFU)||cp>0x10FFFFU)return false;
        i+=n;
    } return true;
}
void append_utf8(std::string& out,std::uint32_t cp){
    if(cp<=0x7F)out.push_back(static_cast<char>(cp));
    else if(cp<=0x7FF){out.push_back(static_cast<char>(0xC0U|(cp>>6U)));out.push_back(static_cast<char>(0x80U|(cp&0x3FU)));}
    else if(cp<=0xFFFF){out.push_back(static_cast<char>(0xE0U|(cp>>12U)));out.push_back(static_cast<char>(0x80U|((cp>>6U)&0x3FU)));out.push_back(static_cast<char>(0x80U|(cp&0x3FU)));}
    else {out.push_back(static_cast<char>(0xF0U|(cp>>18U)));out.push_back(static_cast<char>(0x80U|((cp>>12U)&0x3FU)));out.push_back(static_cast<char>(0x80U|((cp>>6U)&0x3FU)));out.push_back(static_cast<char>(0x80U|(cp&0x3FU)));}
}
int hex(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}

class Parser{
    std::string_view in; ParseOptions opt; Position pos{}; std::optional<Error> err;
    bool end()const{return pos.offset>=in.size();} char peek()const{return end()?'\0':in[pos.offset];}
    char adv(){char c=in[pos.offset++];if(c=='\n'){++pos.line;pos.column=1;}else ++pos.column;return c;}
    void ws(){while(!end()&&(peek()==' '||peek()=='\t'||peek()=='\r'||peek()=='\n'))adv();}
    std::optional<Value> failv(std::string c,std::string m){err=Error{pos,std::move(c),std::move(m)};return std::nullopt;}
    bool literal(std::string_view x){for(char c:x){if(end()||peek()!=c)return false;adv();}return true;}
    std::optional<std::uint16_t> hex4(){if(in.size()-pos.offset<4){failv("JSON_INVALID_UNICODE_ESCAPE","incomplete unicode escape");return std::nullopt;}std::uint16_t v=0;for(int i=0;i<4;++i){int d=hex(peek());if(d<0){failv("JSON_INVALID_UNICODE_ESCAPE","invalid unicode escape");return std::nullopt;}adv();v=static_cast<std::uint16_t>((v<<4U)|static_cast<unsigned>(d));}return v;}
    std::optional<std::string> str(){
        adv(); std::string out;
        while(!end()){
            unsigned char c=static_cast<unsigned char>(peek());
            if(c=='"'){adv();return out;}
            if(c<0x20U){failv("JSON_CONTROL_IN_STRING","control character in string");return std::nullopt;}
            if(c!='\\'){out.push_back(adv());if(out.size()>opt.max_string_bytes){failv("JSON_STRING_TOO_LARGE","string limit");return std::nullopt;}continue;}
            adv(); if(end()){failv("JSON_UNTERMINATED_STRING","unterminated string");return std::nullopt;}
            char e=adv();
            switch(e){
                case '"':out.push_back('"');break;case '\\':out.push_back('\\');break;case '/':out.push_back('/');break;
                case 'b':out.push_back('\b');break;case 'f':out.push_back('\f');break;case 'n':out.push_back('\n');break;case 'r':out.push_back('\r');break;case 't':out.push_back('\t');break;
                case 'u':{
                    auto a=hex4();if(!a)return std::nullopt;std::uint32_t cp=*a;
                    if(cp>=0xD800U&&cp<=0xDBFFU){
                        if(in.size()-pos.offset<6||peek()!='\\'||in[pos.offset+1]!='u'){failv("JSON_UNPAIRED_SURROGATE","unpaired surrogate");return std::nullopt;}
                        adv();adv();auto b=hex4();if(!b)return std::nullopt;
                        if(*b<0xDC00U||*b>0xDFFFU){failv("JSON_UNPAIRED_SURROGATE","unpaired surrogate");return std::nullopt;}
                        cp=0x10000U+((cp-0xD800U)<<10U)+(static_cast<std::uint32_t>(*b)-0xDC00U);
                    } else if(cp>=0xDC00U&&cp<=0xDFFFU){failv("JSON_UNPAIRED_SURROGATE","unpaired surrogate");return std::nullopt;}
                    append_utf8(out,cp);break;
                }
                default:failv("JSON_INVALID_ESCAPE","invalid escape");return std::nullopt;
            }
            if(out.size()>opt.max_string_bytes){failv("JSON_STRING_TOO_LARGE","string limit");return std::nullopt;}
        }
        failv("JSON_UNTERMINATED_STRING","unterminated string");return std::nullopt;
    }
    std::optional<Value> num(){
        std::size_t start=pos.offset;if(peek()=='-')adv();if(end())return failv("JSON_INVALID_NUMBER","bad number");
        if(peek()=='0'){adv();if(!end()&&std::isdigit(static_cast<unsigned char>(peek())))return failv("JSON_INVALID_NUMBER","leading zero");}
        else if(peek()>='1'&&peek()<='9'){while(!end()&&std::isdigit(static_cast<unsigned char>(peek())))adv();}
        else return failv("JSON_INVALID_NUMBER","bad number");
        if(!end()&&peek()=='.'){adv();if(end()||!std::isdigit(static_cast<unsigned char>(peek())))return failv("JSON_INVALID_NUMBER","bad fraction");while(!end()&&std::isdigit(static_cast<unsigned char>(peek())))adv();}
        if(!end()&&(peek()=='e'||peek()=='E')){adv();if(!end()&&(peek()=='+'||peek()=='-'))adv();if(end()||!std::isdigit(static_cast<unsigned char>(peek())))return failv("JSON_INVALID_NUMBER","bad exponent");while(!end()&&std::isdigit(static_cast<unsigned char>(peek())))adv();}
        double v=0; auto s=in.substr(start,pos.offset-start); std::string tmp(s);
        char* ep=nullptr; v=std::strtod(tmp.c_str(),&ep); if(ep!=tmp.c_str()+tmp.size()||!std::isfinite(v))return failv("JSON_INVALID_NUMBER","number out of range");
        return Value{v};
    }
    std::optional<Value> arr(std::size_t depth){
        if(depth>opt.max_nesting) return failv("JSON_NESTING_LIMIT","nesting limit");
        adv();ws();Array a;if(!end()&&peek()==']'){adv();return Value{std::move(a)};}
        while(true){auto v=value(depth);if(!v)return std::nullopt;a.push_back(std::move(*v));ws();if(end())return failv("JSON_EXPECTED_COMMA","unterminated array");if(peek()==']'){adv();break;}if(peek()!=',')return failv("JSON_EXPECTED_COMMA","expected comma");adv();ws();if(!end()&&peek()==']')return failv("JSON_TRAILING_COMMA","trailing comma");}
        return Value{std::move(a)};
    }
    std::optional<Value> obj(std::size_t depth){
        if(depth>opt.max_nesting) return failv("JSON_NESTING_LIMIT","nesting limit");
        adv();ws();Object o;if(!end()&&peek()=='}'){adv();return Value{std::move(o)};}
        while(true){
            if(end()||peek()!='"')return failv("JSON_EXPECTED_KEY","expected key");
            auto k=str();if(!k)return std::nullopt;ws();if(end()||peek()!=':')return failv("JSON_EXPECTED_COLON","expected colon");adv();ws();
            auto v=value(depth);if(!v)return std::nullopt;if(o.contains(*k))return failv("JSON_DUPLICATE_KEY","duplicate key");o.emplace(std::move(*k),std::move(*v));ws();
            if(end()) return failv("JSON_EXPECTED_COMMA","unterminated object");
            if(peek()=='}'){adv();break;}
            if(peek()!=',') return failv("JSON_EXPECTED_COMMA","expected comma");
            adv(); ws();
            if(!end()&&peek()=='}') return failv("JSON_TRAILING_COMMA","trailing comma");
        } return Value{std::move(o)};
    }
    std::optional<Value> value(std::size_t depth){
        if(depth>opt.max_nesting) return failv("JSON_NESTING_LIMIT","nesting limit");
        if(end()) return failv("JSON_EXPECTED_VALUE","expected value");
        if(peek()=='n'){if(!literal("null"))return failv("JSON_INVALID_LITERAL","bad literal");return Value{Null{}};}
        if(peek()=='t'){if(!literal("true"))return failv("JSON_INVALID_LITERAL","bad literal");return Value{true};}
        if(peek()=='f'){if(!literal("false"))return failv("JSON_INVALID_LITERAL","bad literal");return Value{false};}
        if(peek()=='"'){auto x=str();return x?std::optional<Value>{Value{std::move(*x)}}:std::nullopt;}
        if(peek()=='[') return arr(depth+1);
        if(peek()=='{') return obj(depth+1);
        if(peek()=='-'||std::isdigit(static_cast<unsigned char>(peek())))return num();
        return failv("JSON_EXPECTED_VALUE","expected value");
    }
public:
    Parser(std::string_view s,ParseOptions o):in(s),opt(o){}
    ParseResult parse(){
        if(in.size()>opt.max_bytes)return {std::nullopt,Error{pos,"JSON_INPUT_TOO_LARGE","input limit"}};
        if(!valid_utf8(in))return {std::nullopt,Error{pos,"JSON_INVALID_UTF8","invalid UTF-8"}};
        ws();auto v=value(0);if(!v)return {std::nullopt,err};ws();if(!end())return {std::nullopt,Error{pos,"JSON_TRAILING_DATA","trailing data"}};return {std::move(v),std::nullopt};
    }
};
}
ParseResult parse(std::string_view input,ParseOptions opt={}){return detail::Parser(input,opt).parse();}
const Value* find_member(const Value& v,std::string_view key){if(!v.is_object())return nullptr;auto it=v.as_object().find(key);return it==v.as_object().end()?nullptr:&it->second;}
}

namespace d2t::dep {
struct Error{std::string code,message;};
struct Rule{std::vector<std::string> targets,prerequisites;};
struct ParseResult{std::vector<Rule> rules;std::optional<Error> error;bool ok()const{return !error.has_value();}};
ParseResult parse(std::string_view input){
    if(input.find('\0')!=std::string_view::npos)return {{},Error{"DEP_NUL","NUL byte"}};
    std::string logical; logical.reserve(input.size());
    for(std::size_t i=0;i<input.size();++i){
        if(input[i]=='\\'&&i+1<input.size()&&input[i+1]=='\n'){++i;logical.push_back(' ');continue;}
        if(input[i]=='\\'&&i+2<input.size()&&input[i+1]=='\r'&&input[i+2]=='\n'){i+=2;logical.push_back(' ');continue;}
        logical.push_back(input[i]);
    }
    if(!logical.empty()&&logical.back()=='\\')return {{},Error{"DEP_TRAILING_ESCAPE","trailing escape"}};
    ParseResult out; std::istringstream lines(logical);std::string line;
    while(std::getline(lines,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        bool esc=false;std::size_t hash=std::string::npos,colon=std::string::npos;
        for(std::size_t i=0;i<line.size();++i){char c=line[i];if(esc){esc=false;continue;}if(c=='\\'){esc=true;continue;}if(c=='#'){hash=i;break;}if(c==':'&&colon==std::string::npos)colon=i;}
        if(hash!=std::string::npos)line.resize(hash);
        if(line.find_first_not_of(" \t")==std::string::npos)continue;
        esc=false;colon=std::string::npos;for(std::size_t i=0;i<line.size();++i){char c=line[i];if(esc){esc=false;continue;}if(c=='\\'){esc=true;continue;}if(c==':'){colon=i;break;}}
        if(colon==std::string::npos)return {{},Error{"DEP_MISSING_COLON","missing colon"}};
        auto tokens=[](std::string_view s)->std::optional<std::vector<std::string>>{
            std::vector<std::string> result_tokens; std::string cur; bool e=false;
            for(char c:s){if(e){cur.push_back(c);e=false;}else if(c=='\\')e=true;else if(c==' '||c=='\t'){if(!cur.empty()){result_tokens.push_back(cur);cur.clear();}}else cur.push_back(c);}
            if(e) return std::nullopt;
            if(!cur.empty()) result_tokens.push_back(cur);
            return result_tokens;
        };
        auto ts=tokens(std::string_view(line).substr(0,colon));auto ps=tokens(std::string_view(line).substr(colon+1));
        if(!ts||!ps) return {{},Error{"DEP_TRAILING_ESCAPE","trailing escape"}};
        if(ts->empty()) return {{},Error{"DEP_EMPTY_TARGET","empty target"}};
        std::sort(ps->begin(),ps->end());ps->erase(std::unique(ps->begin(),ps->end()),ps->end());out.rules.push_back({std::move(*ts),std::move(*ps)});
    } return out;
}
}

namespace d2t::path {
enum class RootClass{Project,Build,External};
struct NormalizedPath{std::filesystem::path absolute,display;RootClass root_class;};
struct Error{std::string code,message;};
struct Result{std::optional<NormalizedPath> value;std::optional<Error> error;bool ok()const{return value.has_value();}};
bool under(const std::filesystem::path& p,const std::filesystem::path& root){
    auto a=p.begin(),b=root.begin();for(;b!=root.end();++a,++b){if(a==p.end()||*a!=*b)return false;}return true;
}
Result normalize(const std::filesystem::path& input,const std::filesystem::path& project,const std::filesystem::path& build,bool allow_external=false){
    if(input.empty())return {std::nullopt,Error{"PATH_EMPTY","empty path"}};
    if(!project.is_absolute()||!build.is_absolute())return {std::nullopt,Error{"PATH_ROOT_NOT_ABSOLUTE","roots must be absolute"}};
    auto pr=project.lexically_normal(),br=build.lexically_normal();auto abs=(input.is_absolute()?input:pr/input).lexically_normal();
    if(under(abs,pr))return {NormalizedPath{abs,abs.lexically_relative(pr),RootClass::Project},std::nullopt};
    if(under(abs,br))return {NormalizedPath{abs,abs.lexically_relative(br),RootClass::Build},std::nullopt};
    if(allow_external&&abs.is_absolute())return {NormalizedPath{abs,abs,RootClass::External},std::nullopt};
    return {std::nullopt,Error{"PATH_OUTSIDE_ROOT","path outside allowed roots"}};
}
}

namespace d2t::io {
struct ReadResult{std::optional<std::string> data;std::string error;bool ok()const{return data.has_value();}};
ReadResult read_text(const std::filesystem::path& p,std::size_t max_bytes=64U*1024U*1024U){
    std::ifstream f(p,std::ios::binary);if(!f)return {std::nullopt,"cannot open file: "+p.string()};
    f.seekg(0,std::ios::end);auto n=f.tellg();if(n<0)return {std::nullopt,"cannot size file: "+p.string()};if(static_cast<std::uint64_t>(n)>max_bytes)return {std::nullopt,"file too large: "+p.string()};
    f.seekg(0);std::string s(static_cast<std::size_t>(n),'\0');if(!s.empty())f.read(s.data(),static_cast<std::streamsize>(s.size()));if(!f&& !s.empty())return {std::nullopt,"cannot read file: "+p.string()};return {std::move(s),{}};
}
}

namespace d2t::ctest {
struct RegisteredTest{std::string name;std::vector<std::string> command;};
struct Catalogue{std::vector<RegisteredTest> tests;};
struct LoadResult{std::optional<Catalogue> catalogue;std::string error;bool ok()const{return catalogue.has_value();}};
LoadResult parse_catalogue(std::string_view text){
    auto parsed=json::parse(text);if(!parsed.ok())return {std::nullopt,"invalid CTest JSON: "+parsed.error->code};
    const auto& root=*parsed.value;if(!root.is_object())return {std::nullopt,"CTest root must be an object"};
    auto* kind=json::find_member(root,"kind");auto* version=json::find_member(root,"version");auto* tests=json::find_member(root,"tests");
    if(!kind||!kind->is_string()||kind->as_string()!="ctestInfo")return {std::nullopt,"CTest kind must be ctestInfo"};
    if(!version||!version->is_object())return {std::nullopt,"CTest version must be object"};
    auto* major=json::find_member(*version,"major");if(!major||!major->is_number()||major->as_number()!=1.0)return {std::nullopt,"unsupported CTest major version"};
    if(!tests||!tests->is_array())return {std::nullopt,"CTest tests must be array"};
    Catalogue cat;std::set<std::string> names;
    for(const auto& item:tests->as_array()){
        if(!item.is_object())return {std::nullopt,"CTest test entry must be object"};
        auto* name=json::find_member(item,"name");auto* command=json::find_member(item,"command");
        if(!name||!name->is_string()||name->as_string().empty())return {std::nullopt,"CTest test name must be non-empty string"};
        if(!names.insert(name->as_string()).second)return {std::nullopt,"duplicate CTest test name"};
        if(!command||!command->is_array()||command->as_array().empty())return {std::nullopt,"CTest command must be non-empty array"};
        RegisteredTest t;t.name=name->as_string();
        for(const auto& arg:command->as_array()){if(!arg.is_string())return {std::nullopt,"CTest command entries must be strings"};t.command.push_back(arg.as_string());}
        cat.tests.push_back(std::move(t));
    }
    std::sort(cat.tests.begin(),cat.tests.end(),[](const auto&a,const auto&b){return a.name<b.name;});
    return {std::move(cat),{}};
}
LoadResult load(const std::filesystem::path& p){
    auto r=io::read_text(p);if(!r.ok())return {std::nullopt,r.error};return parse_catalogue(*r.data);
}
}

namespace d2t::cmake {
struct Target{std::string id,name,type;std::vector<std::filesystem::path> sources,artifacts;std::vector<std::string> dependencies;};
struct Model{std::filesystem::path source_root,build_root;std::string configuration;std::vector<Target> targets;};
struct LoadResult{std::optional<Model> model;std::string error;bool ok()const{return model.has_value();}};

std::optional<std::filesystem::path> safe_child(const std::filesystem::path& root,std::string_view rel){
    std::filesystem::path p(rel);if(p.is_absolute())return std::nullopt;auto full=(root/p).lexically_normal();if(!path::under(full,root.lexically_normal()))return std::nullopt;return full;
}
LoadResult load_target(const std::filesystem::path& file,Target& t){
    auto r=io::read_text(file);if(!r.ok())return {std::nullopt,r.error};auto p=json::parse(*r.data);if(!p.ok()||!p.value->is_object())return {std::nullopt,"invalid target JSON: "+file.string()};
    auto* id=json::find_member(*p.value,"id");auto* name=json::find_member(*p.value,"name");auto* type=json::find_member(*p.value,"type");
    if(!id||!id->is_string()||!name||!name->is_string()||!type||!type->is_string())return {std::nullopt,"target missing id/name/type"};
    t.id=id->as_string();t.name=name->as_string();t.type=type->as_string();
    if(auto* src=json::find_member(*p.value,"sources");src&&src->is_array())for(const auto& s:src->as_array()){if(!s.is_object())continue;if(auto* x=json::find_member(s,"path");x&&x->is_string())t.sources.emplace_back(x->as_string());}
    if(auto* arts=json::find_member(*p.value,"artifacts");arts&&arts->is_array())for(const auto& a:arts->as_array()){if(!a.is_object())continue;if(auto* x=json::find_member(a,"path");x&&x->is_string())t.artifacts.emplace_back(x->as_string());}
    if(auto* deps=json::find_member(*p.value,"dependencies");deps&&deps->is_array())for(const auto& d:deps->as_array()){if(!d.is_object())continue;if(auto* x=json::find_member(d,"id");x&&x->is_string())t.dependencies.push_back(x->as_string());}
    return {Model{}, {}};
}
LoadResult load(const std::filesystem::path& reply_dir,const std::optional<std::filesystem::path>& explicit_index,const std::optional<std::string>& wanted_config){
    std::filesystem::path index;
    if(explicit_index){index=*explicit_index;if(!index.is_absolute())index=(reply_dir/index).lexically_normal();}
    else {
        std::vector<std::filesystem::path> found;
        std::error_code ec;for(auto it=std::filesystem::directory_iterator(reply_dir,ec);!ec&&it!=std::filesystem::directory_iterator();++it){auto n=it->path().filename().string();if(n.rfind("index-",0)==0&&it->path().extension()==".json")found.push_back(it->path());}
        if(ec) return {std::nullopt,"cannot enumerate CMake reply directory"};
        if(found.size()!=1) return {std::nullopt,"CMake reply index is missing or ambiguous"};
        index=found.front();
    }
    if(!path::under(index.lexically_normal(),reply_dir.lexically_normal()))return {std::nullopt,"CMake index escapes reply directory"};
    auto ir=io::read_text(index);if(!ir.ok())return {std::nullopt,ir.error};auto ip=json::parse(*ir.data);if(!ip.ok()||!ip.value->is_object())return {std::nullopt,"invalid CMake index JSON"};
    const json::Value* cm=nullptr;
    if(auto* reply=json::find_member(*ip.value,"reply");reply&&reply->is_object()){
        for(const auto& [k,v]:reply->as_object())if(k.rfind("codemodel-v2",0)==0&&v.is_object()){cm=&v;break;}
    }
    if(!cm){ if(auto* objs=json::find_member(*ip.value,"objects");objs&&objs->is_array())for(const auto& o:objs->as_array()){if(!o.is_object())continue;auto* kind=json::find_member(o,"kind");if(kind&&kind->is_string()&&kind->as_string()=="codemodel"){cm=&o;break;}}}
    if(!cm)return {std::nullopt,"CMake index has no codemodel object"};
    auto* jf=json::find_member(*cm,"jsonFile");auto* ver=json::find_member(*cm,"version");
    if(!jf||!jf->is_string())return {std::nullopt,"codemodel jsonFile missing"};
    if(ver&&ver->is_object()){auto* maj=json::find_member(*ver,"major");if(!maj||!maj->is_number()||maj->as_number()!=2.0)return {std::nullopt,"unsupported codemodel major version"};}
    auto cmfile=safe_child(reply_dir,jf->as_string());if(!cmfile)return {std::nullopt,"codemodel reference escapes reply directory"};
    auto cr=io::read_text(*cmfile);if(!cr.ok())return {std::nullopt,cr.error};auto cp=json::parse(*cr.data);if(!cp.ok()||!cp.value->is_object())return {std::nullopt,"invalid codemodel JSON"};
    Model model;
    auto* paths=json::find_member(*cp.value,"paths");auto* configs=json::find_member(*cp.value,"configurations");
    if(!paths||!paths->is_object()||!configs||!configs->is_array())return {std::nullopt,"codemodel missing paths/configurations"};
    auto* source=json::find_member(*paths,"source");auto* build=json::find_member(*paths,"build");
    if(!source||!source->is_string()||!build||!build->is_string())return {std::nullopt,"codemodel paths invalid"};
    model.source_root=source->as_string();model.build_root=build->as_string();
    const json::Value* selected=nullptr;
    if(wanted_config){
        for(const auto& c:configs->as_array())if(c.is_object()){auto* n=json::find_member(c,"name");if(n&&n->is_string()&&n->as_string()==*wanted_config){if(selected)return {std::nullopt,"duplicate requested configuration"};selected=&c;}}
        if(!selected)return {std::nullopt,"requested configuration not found"};
    }else{
        if(configs->as_array().size()!=1) return {std::nullopt,"multiple configurations require --configuration"};
        selected=&configs->as_array().front();
    }
    auto* cname=json::find_member(*selected,"name");model.configuration=(cname&&cname->is_string())?cname->as_string():"";
    auto* targets=json::find_member(*selected,"targets");if(!targets||!targets->is_array())return {std::nullopt,"configuration missing targets"};
    std::set<std::string> ids;
    for(const auto& tr:targets->as_array()){
        if(!tr.is_object()) return {std::nullopt,"target reference must be object"};
        auto* tf=json::find_member(tr,"jsonFile");
        if(!tf||!tf->is_string()) return {std::nullopt,"target reference missing jsonFile"};
        auto file=safe_child(reply_dir,tf->as_string());if(!file)return {std::nullopt,"target reference escapes reply directory"};Target t;auto lr=load_target(*file,t);if(!lr.ok())return {std::nullopt,lr.error};if(!ids.insert(t.id).second)return {std::nullopt,"duplicate target id"};model.targets.push_back(std::move(t));
    }
    std::sort(model.targets.begin(),model.targets.end(),[](const auto&a,const auto&b){return a.id<b.id;});
    return {std::move(model),{}};
}
}

namespace d2t::mapping {
struct TestMapping{std::string test_name,target_id;std::filesystem::path executable;};
struct Result{std::vector<TestMapping> mappings;std::vector<std::string> errors;bool complete()const{return errors.empty();}};
Result map_tests(const ctest::Catalogue& cat,const cmake::Model& model,const std::filesystem::path& build_root){
    std::map<std::filesystem::path,std::vector<std::string>> artifacts;
    for(const auto& t:model.targets)if(t.type=="EXECUTABLE")for(const auto& a:t.artifacts){auto p=(a.is_absolute()?a:build_root/a).lexically_normal();artifacts[p].push_back(t.id);}
    Result r;
    for(const auto& test:cat.tests){
        std::filesystem::path exe(test.command.front());if(!exe.is_absolute())exe=(build_root/exe).lexically_normal();else exe=exe.lexically_normal();
        auto it=artifacts.find(exe);if(it==artifacts.end()||it->second.size()!=1){r.errors.push_back("test '"+test.name+"' does not map uniquely to a CMake executable artifact");continue;}
        r.mappings.push_back({test.name,it->second.front(),exe});
    }
    std::sort(r.mappings.begin(),r.mappings.end(),[](const auto&a,const auto&b){return a.test_name<b.test_name;});return r;
}
}

namespace d2t::cli {
struct AnalyzeOptions{
    std::filesystem::path project_root,build_root,cmake_reply,ctest_info,dep_list;
    std::string changed_files;std::optional<std::filesystem::path> cmake_index;std::optional<std::string> configuration;
    std::string format="human";bool explain=false,verbose=false;
};
struct ParseResult{bool ok=false;AnalyzeOptions options;std::string error;};
void print_help(std::ostream& out){
    out<<"diff2test - conservative C++ test-impact analysis\n\nUsage:\n  diff2test --help\n  diff2test --version\n  diff2test analyze [options]\n\n"
       <<"Required analyze options:\n  --project-root <dir>\n  --build-root <dir>\n  --changed-files <file|->\n  --cmake-reply <dir>\n  --ctest-info <file>\n  --dep-list <file>\n\n"
       <<"Optional:\n  --cmake-index <file>\n  --configuration <name>\n  --format <human|names>\n  --explain\n  --verbose\n\n"
       <<"diff2test only reads pre-generated metadata. It never launches Git, CMake,\nCTest, a compiler, a shell, or any other external program.\n";
}
ParseResult parse_analyze(int argc,char** argv){
    ParseResult r;std::map<std::string,std::string> vals;std::set<std::string> flags;
    const std::set<std::string> vo={"--project-root","--build-root","--changed-files","--cmake-reply","--cmake-index","--ctest-info","--dep-list","--configuration","--format"};
    const std::set<std::string> fo={"--explain","--verbose"};
    for(int i=2;i<argc;++i){std::string a=argv[i];if(vo.contains(a)){if(vals.contains(a))return {false,{}, "repeated option: "+a};if(i+1>=argc)return {false,{},"missing value for option: "+a};vals[a]=argv[++i];}
        else if(fo.contains(a)){if(!flags.insert(a).second)return {false,{},"repeated option: "+a};}else return {false,{},"unknown option: "+a};}
    for(auto k:{"--project-root","--build-root","--changed-files","--cmake-reply","--ctest-info","--dep-list"})if(!vals.contains(k))return {false,{},"missing required option: "+std::string(k)};
    if(vals.contains("--format")&&vals["--format"]!="human"&&vals["--format"]!="names")return {false,{},"--format must be either 'human' or 'names'"};
    r.options.project_root=vals["--project-root"];r.options.build_root=vals["--build-root"];r.options.changed_files=vals["--changed-files"];r.options.cmake_reply=vals["--cmake-reply"];r.options.ctest_info=vals["--ctest-info"];r.options.dep_list=vals["--dep-list"];
    if(vals.contains("--cmake-index")) r.options.cmake_index=vals["--cmake-index"];
    if(vals.contains("--configuration")) r.options.configuration=vals["--configuration"];
    if(vals.contains("--format")) r.options.format=vals["--format"];
    r.options.explain=flags.contains("--explain");r.options.verbose=flags.contains("--verbose");r.ok=true;return r;
}
}

namespace d2t {
int run(int argc,char** argv){
    using core::Outcome;
    if(argc==2){std::string_view a=argv[1];if(a=="--help"||a=="-h"){cli::print_help(std::cout);return 0;}if(a=="--version"){std::cout<<"diff2test "<<core::kVersion<<'\n';return 0;}}
    if(argc<2||std::string_view(argv[1])!="analyze"){std::cerr<<"diff2test: expected command 'analyze' (try --help)\n";return core::exit_code(Outcome::UsageError);}
    auto p=cli::parse_analyze(argc,argv);if(!p.ok){std::cerr<<"diff2test: "<<p.error<<'\n';return core::exit_code(Outcome::UsageError);}
    auto cat=ctest::load(p.options.ctest_info);
    if(!cat.ok()){std::cout<<"STATUS: FULL_SUITE_REQUIRED\n";std::cerr<<"diff2test: cannot enumerate registered tests: "<<cat.error<<"\n";return core::exit_code(Outcome::FullSuiteRequired);}
    auto model=cmake::load(p.options.cmake_reply,p.options.cmake_index,p.options.configuration);
    if(!model.ok()){
        if(p.options.format=="human")std::cout<<"STATUS: FULL_SUITE_SELECTED\n";
        for(const auto& t:cat.catalogue->tests)std::cout<<t.name<<'\n';
        std::cerr<<"diff2test: safety fallback: "<<model.error<<"\n";return core::exit_code(Outcome::FullSuiteSelected);
    }
    auto maps=mapping::map_tests(*cat.catalogue,*model.model,p.options.build_root);
    if(!maps.complete()){
        if(p.options.format=="human")std::cout<<"STATUS: FULL_SUITE_SELECTED\n";
        for(const auto& t:cat.catalogue->tests)std::cout<<t.name<<'\n';
        std::cerr<<"diff2test: safety fallback: "<<maps.errors.front()<<"\n";return core::exit_code(Outcome::FullSuiteSelected);
    }
    if(p.options.format=="human")std::cout<<"STATUS: FULL_SUITE_SELECTED\n";
    for(const auto& t:cat.catalogue->tests)std::cout<<t.name<<'\n';
    std::cerr<<"diff2test: safety fallback: dependency evidence analysis is not implemented yet\n";
    return core::exit_code(Outcome::FullSuiteSelected);
}
}

#ifndef D2T_TESTING
int main(int argc,char** argv){
    try{return d2t::run(argc,argv);}catch(const std::exception& e){std::cerr<<"diff2test: internal error: "<<e.what()<<'\n';return d2t::core::exit_code(d2t::core::Outcome::InternalError);}catch(...){std::cerr<<"diff2test: internal error: unknown exception\n";return d2t::core::exit_code(d2t::core::Outcome::InternalError);}
}
#endif
