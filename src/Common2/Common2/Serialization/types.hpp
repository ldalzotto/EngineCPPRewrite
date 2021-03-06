#pragma once

struct PrimitiveSerializedTypes
{
    enum class Type
    {
        UNDEFINED = 0,
        FLOAT32 = UNDEFINED + 1,
        FLOAT32_2 = FLOAT32 + 1,
        FLOAT32_3 = FLOAT32_2 + 1,
        FLOAT32_4 = FLOAT32_3 + 1,
    };

    inline static uimax get_size(const Type p_type)
    {
        switch (p_type)
        {
        case Type::FLOAT32:
            return sizeof(float32);
        case Type::FLOAT32_2:
            return sizeof(float32) * 2;
        case Type::FLOAT32_3:
            return sizeof(float32) * 3;
        case Type::FLOAT32_4:
            return sizeof(float32) * 4;
        default:
            abort();
        }
    };

    inline static PrimitiveSerializedTypes::Type get_type_from_string(const Slice<int8>& p_str)
    {
        if (slice_int8_build_rawstr("FLOAT32").compare(p_str))
        {
            return PrimitiveSerializedTypes::Type::FLOAT32;
        }
        else if (slice_int8_build_rawstr("FLOAT32_2").compare(p_str))
        {
            return PrimitiveSerializedTypes::Type::FLOAT32_2;
        }
        else if (slice_int8_build_rawstr("FLOAT32_3").compare(p_str))
        {
            return PrimitiveSerializedTypes::Type::FLOAT32_3;
        }
        else if (slice_int8_build_rawstr("FLOAT32_4").compare(p_str))
        {
            return PrimitiveSerializedTypes::Type::FLOAT32_4;
        }
        else
        {
            abort();
        }
    };
};
