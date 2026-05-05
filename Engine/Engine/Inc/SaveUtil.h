#pragma once

#include <rapidjson/document.h>

namespace Engine::SaveUtil
{
    bool ReadBool(const char* key, bool& b, const rapidjson::Value& value);
    void WriteBool(const char* key, const bool& b, rapidjson::Document& doc, rapidjson::Value& member);

    bool ReadInt(const char* key, int& i, const rapidjson::Value& value);
    void WriteInt(const char* key, const int& i, rapidjson::Document& doc, rapidjson::Value& member);

    bool ReadFloat(const char* key, float& f, const rapidjson::Value& value);
    void WriteFloat(const char* key, const float& f, rapidjson::Document& doc, rapidjson::Value& member);

    bool ReadVector3(const char* key, Math::Vector3& v, const rapidjson::Value& value);
    void WriteVector3(const char* key, const Math::Vector3& v, rapidjson::Document& doc, rapidjson::Value& member);

    bool ReadQuaternion(const char* key, Math::Quaternion& q, const rapidjson::Value& value);
    void WriteQuaternion(const char* key, const Math::Quaternion& q, rapidjson::Document& doc, rapidjson::Value& member);

    bool ReadColor(const char* key, Graphics::Color& c, const rapidjson::Value& value);
    void WriteColor(const char* key, const Graphics::Color& c, rapidjson::Document& doc, rapidjson::Value& member);

    bool ReadString(const char* key, std::string& str, const rapidjson::Value& value);
    void WriteString(const char* key, const char* str, rapidjson::Document& doc, rapidjson::Value& member);

    bool ReadStringArray(const char* key, std::vector<std::string>& strArray, const rapidjson::Value& value);
    void WriteStringArray(const char* key, const std::vector<std::string>& strArray, rapidjson::Document& doc, rapidjson::Value& member);
}
