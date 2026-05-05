#include "Precompiled.h"
#include "SaveUtil.h"

using namespace Engine;

bool SaveUtil::ReadBool(const char* key, bool& b, const rapidjson::Value& value)
{
    if (value.HasMember(key))
    {
        b = value[key].GetBool();
        return true;
    }
    return false;
}

void SaveUtil::WriteBool(const char* key, const bool& b, rapidjson::Document& doc, rapidjson::Value& member)
{
    rapidjson::GenericStringRef<char> keyStr(key);
    member.AddMember(keyStr, b, doc.GetAllocator());
}

bool SaveUtil::ReadInt(const char* key, int& i, const rapidjson::Value& value)
{
    if (value.HasMember(key))
    {
        i = value[key].GetInt();
        return true;
    }
    return false;
}

void SaveUtil::WriteInt(const char* key, const int& i, rapidjson::Document& doc, rapidjson::Value& member)
{
    rapidjson::GenericStringRef<char> keyStr(key);
    member.AddMember(keyStr, i, doc.GetAllocator());
}

bool SaveUtil::ReadFloat(const char* key, float& f, const rapidjson::Value& value)
{
    if (value.HasMember(key))
    {
        f = value[key].GetFloat();
        return true;
    }
    return false;
}

void SaveUtil::WriteFloat(const char* key, const float& f, rapidjson::Document& doc, rapidjson::Value& member)
{
    rapidjson::GenericStringRef<char> keyStr(key);
    member.AddMember(keyStr, f, doc.GetAllocator());
}

bool SaveUtil::ReadVector3(const char* key, Math::Vector3& v, const rapidjson::Value& value)
{
    if (value.HasMember(key))
    {
        const auto& vec = value[key].GetArray();
        v.x = vec[0].GetFloat();
        v.y = vec[1].GetFloat();
        v.z = vec[2].GetFloat();
        return true;
    }
    return false;
}

void SaveUtil::WriteVector3(const char* key, const Math::Vector3& v, rapidjson::Document& doc, rapidjson::Value& member)
{
    rapidjson::GenericStringRef<char> keyStr(key);
    rapidjson::Value vecArray(rapidjson::kArrayType);
    vecArray.PushBack(v.x, doc.GetAllocator());
    vecArray.PushBack(v.y, doc.GetAllocator());
    vecArray.PushBack(v.z, doc.GetAllocator());
    member.AddMember(keyStr, vecArray, doc.GetAllocator());
}

bool SaveUtil::ReadQuaternion(const char* key, Math::Quaternion& q, const rapidjson::Value& value)
{
    if (value.HasMember(key))
    {
        const auto& quaternion = value[key].GetArray();
        q.x = quaternion[0].GetFloat();
        q.y = quaternion[1].GetFloat();
        q.z = quaternion[2].GetFloat();
        q.w = quaternion[3].GetFloat();
        return true;
    }
    return false;
}

void SaveUtil::WriteQuaternion(const char* key, const Math::Quaternion& q, rapidjson::Document& doc, rapidjson::Value& member)
{
    rapidjson::GenericStringRef<char> keyStr(key);
    rapidjson::Value quatArray(rapidjson::kArrayType);
    quatArray.PushBack(q.x, doc.GetAllocator());
    quatArray.PushBack(q.y, doc.GetAllocator());
    quatArray.PushBack(q.z, doc.GetAllocator());
    quatArray.PushBack(q.w, doc.GetAllocator());
    member.AddMember(keyStr, quatArray, doc.GetAllocator());
}

bool SaveUtil::ReadColor(const char* key, Graphics::Color& c, const rapidjson::Value& value)
{
    if (value.HasMember(key))
    {
        const auto& color = value[key].GetArray();
        c.r = color[0].GetFloat();
        c.g = color[1].GetFloat();
        c.b = color[2].GetFloat();
        c.a = color[3].GetFloat();
        return true;
    }
    return false;
}

void SaveUtil::WriteColor(const char* key, const Graphics::Color& c, rapidjson::Document& doc, rapidjson::Value& member)
{
    rapidjson::GenericStringRef<char> keyStr(key);
    rapidjson::Value colorArray(rapidjson::kArrayType);
    colorArray.PushBack(c.r, doc.GetAllocator());
    colorArray.PushBack(c.g, doc.GetAllocator());
    colorArray.PushBack(c.b, doc.GetAllocator());
    colorArray.PushBack(c.a, doc.GetAllocator());
    member.AddMember(keyStr, colorArray, doc.GetAllocator());
}

bool SaveUtil::ReadString(const char* key, std::string& str, const rapidjson::Value& value)
{
    if (value.HasMember(key))
    {
        str = value[key].GetString();
        return true;
    }
    return false;
}

void SaveUtil::WriteString(const char* key, const char* str, rapidjson::Document& doc, rapidjson::Value& member)
{
    rapidjson::GenericStringRef<char> keyStr(key);
    rapidjson::GenericStringRef<char> valueStr(str);
    member.AddMember(keyStr, valueStr, doc.GetAllocator());
}

bool SaveUtil::ReadStringArray(const char* key, std::vector<std::string>& strArray, const rapidjson::Value& value)
{
    if (value.HasMember(key))
    {
        const auto& strings = value[key].GetArray();
        for (auto& string : strings)
        {
            strArray.push_back(string.GetString());
        }
        return true;
    }
    return false;
}

void SaveUtil::WriteStringArray(const char* key, const std::vector<std::string>& strArray, rapidjson::Document& doc, rapidjson::Value& member)
{
    rapidjson::GenericStringRef<char> keyStr(key);
    rapidjson::Value valueArray(rapidjson::kArrayType);
    for (const std::string& string : strArray)
    {
        rapidjson::GenericStringRef<char> valueStr(string.c_str());
        valueArray.PushBack(valueStr, doc.GetAllocator());
    }
    member.AddMember(keyStr, valueArray, doc.GetAllocator());
}
