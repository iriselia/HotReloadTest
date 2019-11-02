#ifndef GENERATOR_H
#define GENERATOR_H

#include "moc.h"

namespace header_tool {

class Generator
{
    FILE *out;
    ClassDef *cdef;
    std::vector<uint32> meta_data;
public:
    Generator(ClassDef *classDef, const std::vector<std::string> &metaTypes, const std::unordered_map<std::string, std::string> &knownQObjectClasses, const std::unordered_map<std::string, std::string> &knownGadgets, FILE *outfile = 0);
    void generateCode();
private:
    bool registerableMetaType(const std::string &propertyType);
    void registerClassInfoStrings();
    void generateClassInfos();
    void registerFunctionStrings(const std::vector<FunctionDef> &list);
    void generateFunctions(const std::vector<FunctionDef> &list, const char *functype, int type, uint64& paramsIndex);
    void generateFunctionRevisions(const std::vector<FunctionDef> &list, const char *functype);
    void generateFunctionParameters(const std::vector<FunctionDef> &list, const char *functype);
    void generateTypeInfo(const std::string &typeName, bool allowEmptyName = false);
    void registerEnumStrings();
    void generateEnums(uint64 index);
    void registerPropertyStrings();
    void generateProperties();
    void generateMetacall();
    void generateStaticMetacall();
    void generateSignal(FunctionDef *def, int index);
    void generatePluginMetaData();
    std::multimap<std::string, int> automaticPropertyMetaTypesHelper();
    std::map<int, std::multimap<std::string, int> > methodsWithAutomaticTypesHelper(const std::vector<FunctionDef> &methodList);

    void strreg(const std::string &); // registers a string
    uint64 stridx(const std::string & s); // returns a string's id
    std::vector<std::string> strings;
    std::string purestSuperClass;
    std::vector<std::string> metaTypes;
    std::unordered_map<std::string, std::string> knownQObjectClasses;
    std::unordered_map<std::string, std::string> knownGadgets;
};

}

#endif // GENERATOR_H
