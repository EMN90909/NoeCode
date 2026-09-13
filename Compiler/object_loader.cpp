#include "noe.hpp"
#include <array>
#include <fstream>

namespace noe {
namespace {
std::uint16_t le16(const unsigned char*p){return static_cast<std::uint16_t>(p[0])|static_cast<std::uint16_t>(p[1])<<8;}
std::uint32_t le32(const unsigned char*p){return static_cast<std::uint32_t>(p[0])|static_cast<std::uint32_t>(p[1])<<8|static_cast<std::uint32_t>(p[2])<<16|static_cast<std::uint32_t>(p[3])<<24;}
}

std::optional<ObjectFileInfo> ObjectInspector::inspect(const std::filesystem::path&path,Diagnostics&d){
    std::error_code ec;
    const auto size=std::filesystem::file_size(path,ec);
    if(ec){d.error("NQR-K5130",{},"cannot stat object input: "+path.string());return std::nullopt;}
    constexpr std::uint64_t maxObject=512ull*1024ull*1024ull;
    if(size<8||size>maxObject){d.error("NQR-K5131",{},"object size outside accepted bounds: "+std::to_string(size));return std::nullopt;}
    std::ifstream in(path,std::ios::binary);if(!in){d.error("NQR-K5130",{},"cannot open object input: "+path.string());return std::nullopt;}
    std::array<unsigned char,64>h{};in.read(reinterpret_cast<char*>(h.data()),static_cast<std::streamsize>(std::min<std::uint64_t>(h.size(),size)));
    const auto read=static_cast<std::size_t>(in.gcount());if(read<8){d.error("NQR-K5131",{},"truncated object header");return std::nullopt;}
    ObjectFileInfo info;info.size=size;
    if(h[0]==0x7f&&h[1]=='E'&&h[2]=='L'&&h[3]=='F'){
        if(read<20||h[4]!=2||h[5]!=1){d.error("NQR-K5132",{},"unsupported ELF object: Noqeri accepts little-endian ELF64 only");return std::nullopt;}
        info.format=ObjectFormat::Elf64;const auto machine=le16(h.data()+18);if(machine==62)info.architecture=Architecture::X86_64;else if(machine==183)info.architecture=Architecture::AArch64;else{d.error("NQR-K5133",{},"unsupported ELF machine: "+std::to_string(machine));return std::nullopt;}return info;
    }
    if(h[0]==0x00&&h[1]==0x61&&h[2]==0x73&&h[3]==0x6d){
        if(h[4]!=1||h[5]!=0||h[6]!=0||h[7]!=0){d.error("NQR-K5134",{},"unsupported WebAssembly object version");return std::nullopt;}info.format=ObjectFormat::Wasm;info.architecture=Architecture::Wasm32;return info;
    }
    const auto magic=le32(h.data());
    if(magic==0xfeedfacf){
        if(read<12){d.error("NQR-K5131",{},"truncated Mach-O header");return std::nullopt;}
        info.format=ObjectFormat::MachO64;const auto cpu=le32(h.data()+4);if(cpu==0x01000007)info.architecture=Architecture::X86_64;else if(cpu==0x0100000c)info.architecture=Architecture::AArch64;else{d.error("NQR-K5135",{},"unsupported Mach-O CPU type");return std::nullopt;}return info;
    }
    const auto machine=le16(h.data());
    if(machine==0x8664||machine==0xaa64){
        if(read<20){d.error("NQR-K5131",{},"truncated COFF header");return std::nullopt;}
        const auto sections=le16(h.data()+2);const auto optional=le16(h.data()+16);
        if(sections==0||sections>32768||optional>4096){d.error("NQR-K5136",{},"invalid COFF header bounds");return std::nullopt;}
        info.format=ObjectFormat::Coff64;info.architecture=machine==0x8664?Architecture::X86_64:Architecture::AArch64;return info;
    }
    d.error("NQR-K5137",{},"unrecognized or unsupported object format: "+path.string());return std::nullopt;
}

} // namespace noe
