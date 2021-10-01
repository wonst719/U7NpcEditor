#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <cstdint>

typedef uint8_t byte;

#pragma pack(push, 1)

struct Vector
{
    uint16_t item1;
    uint16_t item2;
};

struct XY
{
    byte X;
    byte Y;
};

struct Actor
{
    XY xy;
    uint16_t shnum;
    uint16_t iflag1;
    byte schunk;
    byte map_num;
    uint16_t usefun;
    int8_t health;
    char unk[3];
    uint16_t iflag2;
    uint16_t rflags;
    byte str;
    byte dex;
    byte intel;
    byte combat;
    byte schedule_type;
    byte amode;
    byte charmalign; // U7: UNK
    byte unk0;
    byte unk1;
    byte temp;   // U7: magic
    byte flags3; // U7: mana
    uint16_t face_num;
    byte unk2;
    uint32_t exp;
    byte training;
    uint16_t primary_attacker;
    uint16_t secondary_attacker;
    uint16_t oppressor;
    Vector unk3_i_vr;
    Vector schedule_loc;
    uint16_t tflags;
    char unk4[5];
    byte next_schedule;
    byte unk5;
    uint16_t unk6;
    uint16_t unk7;
    uint16_t shnum2;
    uint16_t polymorph_shnum;   // U7: skip

    // Skip
    char skip[4 + 2 + 4 + 1];
    char skip2[14];
    int8_t food_read;
    char skip3[7];
    char name[16];
};

#pragma pack(pop)

std::vector<byte> ReadFile(const std::string& fileName)
{
    std::basic_ifstream<byte> is(fileName, std::basic_ifstream<byte>::binary);

    std::vector<byte> buffer(std::istreambuf_iterator<byte>(is), {});

    return buffer;
}

void WriteFile(const std::string fileName, const std::vector<byte>& vec)
{
    std::basic_ofstream<byte> os(fileName, std::basic_ofstream<byte>::binary);

    int outLength = vec.size();
    const byte* bytes = vec.data();
    os.write(bytes, outLength);
}

void ReplaceNpcName(int i, byte*& bytes, const char* replacedName)
{
    if (strlen(replacedName) > 16)
    {
        printf("ERROR: len(%s) > 16\n", replacedName);
        exit(0);
    }

    Actor* actor = (Actor*)bytes;

    char name[17] = { 0, };

    // dbg
    memcpy(name, actor->name, 16);
    printf("%04x %s\n", i, name);

    memset(name, 0, sizeof(name));

    strcpy_s(name, strlen(replacedName) + 1, replacedName);

    // replace name
    memcpy(actor->name, name, 16);

    // dbg
    //memcpy(name, actor->name, 16);
    //printf("%d: %s\n", i, name);

    bytes += sizeof(Actor);

    int num = 0;
    bool unused = actor->iflag2 == 0;
    bool has_contents = actor->iflag1 && !unused;
    if (has_contents)
    {
        // ireg
        for (int entryLen = *bytes++; ; entryLen = *bytes++)
        {
            if (entryLen == 0)
                break;
            else if (entryLen == 1)
                continue;
            else if (entryLen == 255)
            {
                // Special Ireg
                // initdata에는 등장하지 않음.
                continue;
            }
            else if (entryLen == 254 || entryLen == 253)
            {
                // initdata에는 등장하지 않음.
                entryLen = *bytes++;
            }

            bytes += entryLen;
        }
    }
}

std::vector<std::string> ReadTrans(const std::string& fileName)
{
    std::vector<std::string> nameVec;

    FILE* koreanNameFile;
    errno_t err = fopen_s(&koreanNameFile, fileName.c_str(), "rb");
    if (err != 0)
    {
        throw std::exception("Could not open trans");
    }

    int i = 0;
    while (!feof(koreanNameFile))
    {
        char lineBuf[256 + 1] = { 0, };
        fgets(lineBuf, 256, koreanNameFile);
        if (strlen(lineBuf) > 0 && (lineBuf[strlen(lineBuf) - 1] == '\r' || lineBuf[strlen(lineBuf) - 1] == '\n'))
            lineBuf[strlen(lineBuf) - 1] = 0;
        if (strlen(lineBuf) > 0 && (lineBuf[strlen(lineBuf) - 1] == '\r' || lineBuf[strlen(lineBuf) - 1] == '\n'))
            lineBuf[strlen(lineBuf) - 1] = 0;
        int id;
        sscanf_s(lineBuf, "%x", &id);

        if (id != i)
        {
            throw std::exception("ID mismatch");
        }
        char* split = strchr(lineBuf, ' ');
        if (split == nullptr)
        {
            nameVec.push_back("");
        }
        else
        {
            nameVec.push_back(split + 1);
        }

        i++;
    }

    fclose(koreanNameFile);

    return nameVec;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("Usage: exec -i input -t trans -o output\n");
        return 0;
    }

    bool inputIsSet = false;
    bool transIsSet = false;
    bool outputIsSet = false;
    std::string inputFileName;
    std::string transFileName;
    std::string outputFileName;
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--input") && argc > i + 1)
        {
            inputFileName = argv[i + 1];
            inputIsSet = true;
        }
        else if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--trans"))
        {
            transFileName = argv[i + 1];
            transIsSet = true;
        }
        else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output"))
        {
            outputFileName = argv[i + 1];
            outputIsSet = true;
        }
    }

    std::vector<std::string> nameVec = ReadTrans(transFileName);

    std::vector<byte> file = ReadFile(inputFileName);

    byte* bytes = file.data();

    // npc.dat
    bytes += 13;

    uint16_t num_npcs1 = *(uint16_t*)bytes;
    bytes += 2;
    uint16_t num_npcs = num_npcs1 + *(uint16_t*)bytes;
    bytes += 2;

    for (int i = 0; i < num_npcs; i++)
        ReplaceNpcName(i, bytes, nameVec[i].c_str());

    WriteFile(outputFileName, file);

    return 0;
}
