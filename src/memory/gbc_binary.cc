#include "gbc_binary.h" //GBCBinary
#include "../util/util.h" //Util
#include <algorithm> //std::equal
#include <stdexcept> //std::out_of_range
#include <cstring> //std::memcpy
#include <sstream> //std::ostringstream
#include <iomanip> //std::hex, std::setw, std::setfill
#include <map> //std::map

///@brief Static function that parses the given byte buffer as a GBCBinary.
///@details Extracts header data and validates the nintendo logo from the given byte array. Returns info in the form of GBCBinary object.
///@param byte_buffer std::vector buffer containing the binary bytes.
///@return Parsed GBCBinary ready to be used.
///@throw std::out_of_range If given array is too small to be valid GBCBinary.
GBCBinary GBCBinary::parse_bytes(const std::vector<unsigned char>& byte_buffer){
    return GBCBinary(
        GBCBinary::extract_header_data(byte_buffer),
        GBCBinary::valid_nintendo_logo(byte_buffer),
        GBCBinary::valid_header_checksum(byte_buffer),
        byte_buffer
    );
}

///@brief Checks if the nintendo logo is correct in byte_buffer.
///@details Checks wheter the bytes 0x104=>0x133 are a valid nintendo logo.
///@param byte_buffer std::vector buffer containing the binary bytes.
///@return Were the bytes 0x104=>0x133 present and presented a valid nintendo logo?
///@throw std::out_of_range If given array is too small to contain logo.
bool GBCBinary::valid_nintendo_logo(const std::vector<uint8_t>& byte_buffer){
    const std::vector<uint8_t> nintendo_logo_bytes = {
        0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03,
        0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08,
        0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E,
        0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
        0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 
        0xB9, 0x33, 0x3E
    };
    //Nintendo logo located at range 0x104 => 0x133
    const uint16_t logo_start_addr = 0x104;
    const uint16_t logo_end_addr = 0x133;
    if(byte_buffer.size() >= logo_end_addr){
        if (
            std::equal(
                (byte_buffer.begin() + logo_start_addr), 
                (byte_buffer.begin() + logo_end_addr),
                nintendo_logo_bytes.begin()
            )
        ){
            return true;
        }
        return false;
    }
    //Not valid GBC binary
    throw std::out_of_range("Given binary is not valid GBCBinary. Missing logo byte ranges 0x104 to 0x133.");
}

///@brief Checks if the header checksum is correct in byte_buffer.
///@details Checks wheter the header bytes 0x134=>0x14C are a valid using the checksum at 0x14D.
///@param byte_buffer std::vector buffer containing the binary bytes.
///@return Checks wheter the header bytes 0x134=>0x14C are a valid using the checksum at 0x14D.
///@throw std::out_of_range If given array is too small to contain header.
bool GBCBinary::valid_header_checksum(const std::vector<uint8_t>& byte_buffer){
    const uint16_t header_end_addr = 0x14F;
    if(byte_buffer.size() >= header_end_addr){
        //From wiki: x=0:FOR i=0134h TO 014C h:x=x-MEM[i]-1:NEXT
        const uint16_t header_check_start_addr = 0x134;
        const uint16_t header_check_end_addr = 0x14C;
        const uint16_t header_checksum_addr = 0x14D;
        uint8_t calculated_checksum = 0x0;
        for(uint16_t cur_header_flag = header_check_start_addr; cur_header_flag <= header_check_end_addr; ++cur_header_flag){
            calculated_checksum = calculated_checksum - byte_buffer[cur_header_flag] - 1;
        }
        if(calculated_checksum == byte_buffer[header_checksum_addr]){
            return true;
        }
        return false;
    }
    //Not valid GBC binary
    throw std::out_of_range("Given binary is not valid GBCBinary. Missing header data at byte ranges 0x134 to 0x14F.");
}

///@brief Extracts binary headerdata.
///@details Extracts all the binary headerdata available.
///@param byte_buffer std::vector buffer containing the binary bytes.
///@return headerdata available at 0x134 to 0x14F
///@throw std::out_of_range If given array is too small to contain header data.
GBCBinary::gbc_binary_headerdata GBCBinary::extract_header_data(const std::vector<uint8_t>& byte_buffer){
    const uint16_t header_end_addr = 0x14F;
    if(byte_buffer.size() >= header_end_addr){
        GBCBinary::gbc_binary_headerdata header_data;
        const uint16_t header_title_start_addr = 0x134;
        const uint16_t header_title_end_addr = 0x142;
        const std::map<std::string, uint16_t> header_flag_addr = {
            {"gameboy_type", 0x143}, //gameboy_type, 1 byte
            {"licencee_new_byte_1", 0x144}, //licencee_new, 2 bytes, byte 1/2
            {"licencee_new_byte_2", 0x145}, //licencee_new, 2 bytes, byte 2/2
            {"sgb_compatability", 0x146}, //sgb_compatability, 1 byte
            {"cartridge_type", 0x147}, //cartridge_type, 1 byte
            {"rom_size", 0x148}, //rom_size, 1 byte
            {"ram_size", 0x149}, //ram_size, 1 byte
            {"japanese_code", 0x14A}, //japanese_code, 1 byte
            {"licencee_old", 0x14B}, //licencee_old, 1 byte
            {"mask_rom_version", 0x14C}, //mask_rom_version, 1 byte
            {"complement_check", 0x14D}, //complement_check, 1 byte
            {"checksum", 0x14E}, //checksum, 2 bytes
        };
        //Handle title as complex data type std::string
        header_data.title = std::string(
            reinterpret_cast<const char*>(
                &byte_buffer[header_title_start_addr]
            ),
            (header_title_end_addr - header_title_start_addr)
        );
        //Copy header flags
        header_data.gameboy_type = byte_buffer[header_flag_addr.at("gameboy_type")];
        try{
            header_data.licencee_new = Util::combined_char_based_value(
                byte_buffer[header_flag_addr.at("licencee_new_byte_1")],
                byte_buffer[header_flag_addr.at("licencee_new_byte_2")]
            );
        }
        catch(const std::out_of_range & e){
            //Ignore any interpetation errors and mark as 0 (None).
            header_data.licencee_new = 0;
        }
        header_data.sgb_compatability = byte_buffer[header_flag_addr.at("sgb_compatability")];
        header_data.cartridge_type = byte_buffer[header_flag_addr.at("cartridge_type")];
        header_data.rom_size = byte_buffer[header_flag_addr.at("rom_size")];
        header_data.ram_size = byte_buffer[header_flag_addr.at("ram_size")];
        header_data.japanese_code = byte_buffer[header_flag_addr.at("japanese_code")];
        header_data.licencee_old = byte_buffer[header_flag_addr.at("licencee_old")];
        header_data.mask_rom_version = byte_buffer[header_flag_addr.at("mask_rom_version")];
        header_data.complement_check = byte_buffer[header_flag_addr.at("complement_check")];
        std::memcpy(&header_data.checksum, &byte_buffer[header_flag_addr.at("checksum")], sizeof(uint16_t));
        header_data.checksum = Util::nthos16_t(header_data.checksum);
        return header_data;
    }
    //Not valid GBC binary
    throw std::out_of_range("Given binary is not valid GBCBinary. Missing header data at byte ranges 0x134 to 0x14F.");
}

///@brief Initializes empty GBCBinary.
///@details Initializes empty GBCBinary.
GBCBinary::GBCBinary():AddressableMemory(), binary_header_data_(), header_is_valid_(false), has_valid_nintendo_logo_(false){
}

///@brief Initializes GBCBinary with values.
///@param header Parsed header data of the binary.
///@param valid_logo Logo status of the binary.
///@param valid_header Was header validated succesfull using the checksum?
///@param byte_buffer Bytes of the binary.
///@details Initializes GBCBinary with values.
GBCBinary::GBCBinary(const GBCBinary::gbc_binary_headerdata& header, const bool valid_logo, const bool valid_header, const std::vector<uint8_t>& byte_buffer)
:AddressableMemory(byte_buffer, false), binary_header_data_(header), header_is_valid_(valid_header), has_valid_nintendo_logo_(valid_logo)
{
}

///@brief Getter for the binary headerdata variable (see struct `gbc_binary_headerdata`).
///@details Returns the headerdata available for the binary.
///@return headerdata available for the binary (see struct `gbc_binary_headerdata`).
const GBCBinary::gbc_binary_headerdata& GBCBinary::get_header_data() const{
    return binary_header_data_;
}

///@brief Does the binary have a valid nintendo logo?
///@details Does the 0x104 => 0x133 segtion represent a valid nintendo logo?
///@return Is the nintendo logo valid?
const bool& GBCBinary::is_valid_nintendo_logo() const{
    return has_valid_nintendo_logo_;
}

///@brief Gets the logo status and header data as a string representation.
///@brief Gets the logo status and header data as a string representation. Does not include byte contents of binary.
///@return Binary header and logo status represented as string.
std::string GBCBinary::to_string() const{
    std::ostringstream str_builder;
    str_builder << "Binary size in bytes: " << get_memory().size() << "\n";
    str_builder << "Logo status: " << (has_valid_nintendo_logo_ ? "valid" : "not valid") << "\n";
    str_builder << "Header status: " << (header_is_valid_ ? "valid" : "not valid") << "\n";
    str_builder << "Binary title: " << binary_header_data_.title << "\n";
    str_builder << std::hex;
    //Cast to int, uint8_t might be treated as a char otherwise
    str_builder << "Binary gameboy type: " << std::setw(2) << std::setfill('0') << static_cast<int>(binary_header_data_.gameboy_type) << "\n";
    str_builder << "Binary licencee new: " << std::setw(2) << std::setfill('0') << static_cast<int>(binary_header_data_.licencee_new) << "\n";
    str_builder << "Binary sgb compatability: " << std::setw(2) << std::setfill('0') << static_cast<int>(binary_header_data_.sgb_compatability) << "\n";
    str_builder << "Binary cartridge type: " << std::setw(2) << std::setfill('0') << static_cast<int>(binary_header_data_.cartridge_type) << "\n";
    str_builder << "Binary rom size: " << std::setw(2) << std::setfill('0') << static_cast<int>(binary_header_data_.rom_size) << "\n";
    str_builder << "Binary ram size: " << std::setw(2) << std::setfill('0') << static_cast<int>(binary_header_data_.ram_size) << "\n";
    str_builder << "Binary japanese code: " << std::setw(2) << std::setfill('0') << static_cast<int>(binary_header_data_.japanese_code) << "\n";
    str_builder << "Binary licencee old: " << std::setw(2) << std::setfill('0') << static_cast<int>(binary_header_data_.licencee_old) << "\n";
    str_builder << "Binary mask rom version: " << std::setw(2) << std::setfill('0') << static_cast<int>(binary_header_data_.mask_rom_version) << "\n";
    str_builder << "Binary complement check: " << std::setw(2) << std::setfill('0') << static_cast<int>(binary_header_data_.complement_check) << "\n";
    str_builder << "Binary checksum: " << std::setw(4) << std::setfill('0') << binary_header_data_.checksum << "\n";
    return str_builder.str();
}