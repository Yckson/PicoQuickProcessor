#include <bits/stdc++.h>

#define LINE_HEADER(c, addr) \
    std::setfill(' ') << std::setw(8) << std::dec \
    << (c) << ':' << std::setfill('0') << std::setw(4) << std::hex << (addr) << ':' << std::dec

#define PRINT_MOVI(c, addr, rx, i) \
     "mov_r" << (int) (rx) << ',' << std::hex << std::setfill('0') << std::setw(8) << (int) (i)

#define PRINT_MOVR(c, addr, rx, ry) \
     "mov_r" << (int) (rx) << ",r" << (int) (ry)

#define PRINT_MOVRM(c, addr, rx, ry) \
     "mov_r" << (int) (rx) << ",[r" << (int) (ry) << ']'

#define PRINT_MOVMR(c, addr, rx, ry) \
     "mov_[r" << (int) (rx) << "],r" << (int) (ry)

#define PRINT_CMP(c, addr, rx, ry) \
     "cmp_r" << (int) (rx) << "<=>r" << (int) (ry)

#define PRINT_JMP(c, addr, jmp) \
    "jmp_" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)(jmp)

#define PRINT_JG(c, addr, jmp) \
    "jg_" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)(jmp)

#define PRINT_JL(c, addr, jmp) \
    "jl_" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)(jmp)

#define PRINT_JE(c, addr, jmp) \
    "je_" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)(jmp)

#define PRINT_ADD(c, addr, rx, ry) \
     "add_r" << (int) (rx) << "+=r" << (int) (ry)

#define PRINT_SUB(c, addr, rx, ry) \
     "sub_r" << (int) (rx) << "-=r" << (int) (ry)

#define PRINT_AND(c, addr, rx, ry) \
     "and_r" << (int) (rx) << "&=r" << (int) (ry)

#define PRINT_OR(c, addr, rx, ry) \
     "or_r" << (int) (rx) << "|=r" << (int) (ry)

#define PRINT_XOR(c, addr, rx, ry) \
     "xor_r" << (int) (rx) << "^=r" << (int) (ry)

#define PRINT_SAL(c, addr, rx, i) \
     "sal_r" << (int) (rx) << "<<=" << (int) (i)

#define PRINT_SAR(c, addr, rx, i) \
     "sar_r" << (int) (rx) << ">>=" << (int) (i)

#define PRINT_REG(rx)\
    'r' << std::dec << (rx) << '=' << std::hex << std::setfill('0') << std::setw(8)

namespace PQP {

    enum OPCODE : uint8_t {
        MOVI,               // MOV Rx   ,   i16      SIGN-EXTENDED
        MOVR,               // MOV Rx   ,   Ry
        MOVRM,              // MOV Rx   ,   [Ry]
        MOVMR,              // MOV [Rx] ,   [Ry]
        CMP,                // CMP Rx   ,   Ry
        JMP,                // JMP          i16      SIGN-EXTENDED + 4
        JG,                 // JG           i16      SIGN-EXTENDED + 4
        JL,                 // JL           i16      SIGN-EXTENDED + 4
        JE,                 // JE           i16      SIGN-EXTENDED + 4
        ADD,                // ADD Rx   ,   Ry
        SUB,                // SUB Rx   ,   Ry
        AND,                // AND Rx   ,   Ry
        OR,                 // OR  Rx   ,   Ry
        XOR,                // XOR Rx   ,   Ry
        SAL,                // Rx           i5       
        SAR                 // Rx           i5       
    };

    struct INSTRUCTION {
        OPCODE opcode;
        uint8_t Rx;
        uint8_t Ry;
        int16_t i16;
        uint8_t i5;

        INSTRUCTION (uint8_t raw_instruction [4]){
            this->opcode = (OPCODE) raw_instruction[0];
            this->Rx  = raw_instruction[1] >> 4;
            this->Ry  = raw_instruction[1] & 0x0F;
            this->i16 = (int16_t) (*((uint16_t*) &raw_instruction[2]));
            this->i5  = raw_instruction[3] & 0x0F;
        }
    };

    static inline bool get_raw_instruction (std::ifstream& input, uint8_t raw_instruction [4]){
        for (int i = 0; i < 4; i++){
            uint32_t n;
            if (!(input >> std::hex >> n)) return false;
            raw_instruction[i] = (uint8_t) n;
        }
        return true;
    }

    uint8_t pqp_mem   [256];
    uint32_t pqp_addr_counter [256];
    int32_t pqp_regs  [16] ;
    uint8_t pqp_flags =   0;
    uint32_t PC       =   0;

    std::map<uint32_t, std::pair<uint32_t, std::string>> mapped_instructions;

    void load_memory (std::ifstream& input){
        uint8_t raw_instruction [4];

        std::cout << "Loading PQP memory (256 Bytes)..." << std::endl;
        int i;
        for (i = 0; i < 256; i+=4){
            if (!get_raw_instruction(input, raw_instruction)) break;
            pqp_mem[i]   = raw_instruction[0];
            pqp_mem[i+1] = raw_instruction[1];
            pqp_mem[i+2] = raw_instruction[2];
            pqp_mem[i+3] = raw_instruction[3];
        }

        while (i++ < 256){
            pqp_mem[i] = 0;
        }

        std::cout << "Memory loaded! Address: 0x" << std::hex << (uint64_t) pqp_mem << std::endl;
    }
}

namespace JIT {

    enum X86_REGS {
        EAX, ECX, EDX,  EBX,  ESP,  EBP,  ESI,  EDI,
        R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D
    };

    namespace MOV_TYPES {
        enum MOV_TYPES{
            R_I, R_R, R_M, M_R
        };
    }

    namespace REG_TYPE {
        enum REG_TYPE{
            R_OLD = 7,
            R_NEW = 8
        };
    }

    template <MOV_TYPES::MOV_TYPES T>
    size_t emmit_MOV (PQP::INSTRUCTION& instruction, uint8_t* gen_code){
        size_t pos = 0;

        switch (T) {
            case MOV_TYPES::R_I: {
                if (instruction.Rx >= REG_TYPE::R_NEW) gen_code[pos++] = 0x41; //REX.B
                gen_code[pos++] = 0xB8 + (instruction.Rx & 0x7);
                *((int32_t*) &gen_code[pos]) = (int32_t) instruction.i16;
                pos += sizeof(int32_t);
                break;
            }

            case MOV_TYPES::R_R: {
                if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry <= REG_TYPE::R_OLD){
                    gen_code[pos++] = 0x41; // REX.B
                }
                else if (instruction.Rx <= REG_TYPE::R_OLD && instruction.Ry >= REG_TYPE::R_NEW){
                    gen_code[pos++] = 0x44; // REX.R
                }
                else if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry >= REG_TYPE::R_NEW){
                    gen_code[pos++] = 0x45; // REX.R | REX.B
                }

                gen_code[pos++] = 0x89;
                gen_code[pos++] = 0xC0 + (instruction.Rx & 0x7) + ((instruction.Ry & 0x7) << 3);
                break;
            }

            case MOV_TYPES::R_M: {
                gen_code[pos++] = 0x67;

                if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry <= REG_TYPE::R_OLD){
                    gen_code[pos++] = 0x44; //REX.R
                }
                else if (instruction.Rx <= REG_TYPE::R_OLD && instruction.Ry >= REG_TYPE::R_NEW){
                    gen_code[pos++] = 0x41; //REX.B
                }
                else if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry >= REG_TYPE::R_NEW){
                    gen_code[pos++] = 0x45; //REX.R | REX.B
                }

                gen_code[pos++] = 0x8B;
                gen_code[pos++] = 0x80 + (instruction.Ry & 0x7) + ((instruction.Rx & 0x7) << 3);
                *((int32_t*) &gen_code[pos]) = 0;
                pos += 4;
                break;
            }
            case MOV_TYPES::M_R: {
                gen_code[pos++] = 0x67;

                if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry <= REG_TYPE::R_OLD){
                    gen_code[pos++] = 0x41;
                }
                else if (instruction.Rx <= REG_TYPE::R_OLD && instruction.Ry >= REG_TYPE::R_NEW){
                    gen_code[pos++] = 0x44;
                }
                else if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry >= REG_TYPE::R_NEW){
                    gen_code[pos++] = 0x45;
                }

                gen_code[pos++] = 0x89;
                gen_code[pos++] = 0x80 + (instruction.Rx & 0x7) + ((instruction.Ry & 0x7) << 3);
                *((int32_t*) &gen_code[pos]) = 0;
                pos += 4;
                break;
            }
        }
        return pos;
    }

    // Emissor unificado para operações lógicas e aritméticas (ADD, SUB, AND, OR, XOR, CMP)
    template <PQP::OPCODE OP>
    size_t emmit_BINARY_OP (PQP::INSTRUCTION& instruction, uint8_t* gen_code){
        size_t pos = 0;

        // Regra do REX para OP r/m32, r32 (Onde Rx é r/m32 e Ry é r32)
        if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry <= REG_TYPE::R_OLD){
            gen_code[pos++] = 0x41; // REX.B (Rx estendido)
        }
        else if (instruction.Rx <= REG_TYPE::R_OLD && instruction.Ry >= REG_TYPE::R_NEW){
            gen_code[pos++] = 0x44; // REX.R (Ry estendido)
        }
        else if (instruction.Rx >= REG_TYPE::R_NEW && instruction.Ry >= REG_TYPE::R_NEW){
            gen_code[pos++] = 0x45; // REX.R | REX.B (Ambos)
        }

        // Emitir o Opcode correto com base na instrução
        switch (OP) {
            case PQP::ADD: gen_code[pos++] = 0x01; break;
            case PQP::SUB: gen_code[pos++] = 0x29; break;
            case PQP::AND: gen_code[pos++] = 0x21; break;
            case PQP::OR:  gen_code[pos++] = 0x09; break;
            case PQP::XOR: gen_code[pos++] = 0x31; break;
            case PQP::CMP: gen_code[pos++] = 0x39; break;
            default: break;
        }

        // ModR/M: Mod = 11 (0xC0), Reg = Ry, R/M = Rx
        gen_code[pos++] = 0xC0 + (instruction.Rx & 0x7) + ((instruction.Ry & 0x7) << 3);

        return pos;
    }

    // Emissor unificado para operações de deslocamento de bits (SAL, SAR)
    template <PQP::OPCODE OP>
    size_t emmit_SHIFT_OP (PQP::INSTRUCTION& instruction, uint8_t* gen_code){
        size_t pos = 0;

        // O Rx é o destino e fica no campo R/M, logo controla o REX.B
        if (instruction.Rx >= REG_TYPE::R_NEW){
            gen_code[pos++] = 0x41; // REX.B
        }

        // Opcode base para shift com imediato
        gen_code[pos++] = 0xC1;

        // O campo Reg atua como extensão do opcode (SAL = 4, SAR = 7)
        if constexpr (OP == PQP::SAL) {
            gen_code[pos++] = 0xE0 + (instruction.Rx & 0x7); // 0xC0 + (4 << 3) = 0xE0
        } 
        else if constexpr (OP == PQP::SAR) {
            gen_code[pos++] = 0xF8 + (instruction.Rx & 0x7); // 0xC0 + (7 << 3) = 0xF8
        }

        // Valor imediato
        gen_code[pos++] = instruction.i5;

        return pos;
    }

    std::string to_string(uint8_t* gen_code, size_t size) {
        std::ostringstream s;
        for (size_t i = 0; i < size; i++){
            s << "0x" << std::hex << std::setfill('0') << std::setw(2)
            << (int) gen_code[i] << ' ';
        }
        return s.str();
    }
}

std::ifstream input_file;
std::ofstream output_file;

void interpreter (){

    using namespace PQP;
    while (PC < 256){
        
        INSTRUCTION i (&pqp_mem[PC]);
        switch (i.opcode) {
            case MOVI: {
                pqp_regs[i.Rx] = (int32_t) i.i16;
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else { 
                    std::stringstream s;
                    s << PRINT_MOVI (++pqp_addr_counter[PC], PC, i.Rx, i.i16);
                    mapped_instructions[PC] = {1, s.str()};

                    uint8_t gen_code[16];
                    size_t size = JIT::emmit_MOV<JIT::MOV_TYPES::R_I>(i, gen_code);
                    std::string i_text = JIT::to_string(gen_code, size);
                    std::cout << "MOV_I: " << i_text << std::endl;
                }
                break;
            }
            case MOVR: {
                pqp_regs[i.Rx] = pqp_regs[i.Ry];
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_MOVR(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};

                    uint8_t gen_code[16];
                    size_t size = JIT::emmit_MOV<JIT::MOV_TYPES::R_R>(i, gen_code);
                    std::string i_text = JIT::to_string(gen_code, size);
                    std::cout << "MOV_R: " << i_text << std::endl;
                }
                break;
            }
            case MOVRM: {
                pqp_regs[i.Rx] = *((uint32_t*) (&pqp_mem[pqp_regs[i.Ry]]));
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_MOVRM(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};
                    uint8_t gen_code[16];
                    size_t size = JIT::emmit_MOV<JIT::MOV_TYPES::R_M>(i, gen_code);
                    std::string i_text = JIT::to_string(gen_code, size);
                    std::cout << "MOV_R_M: " << i_text << std::endl;
                }
                break;
            }
            case MOVMR: {
                *((uint32_t*) &pqp_mem[pqp_regs[i.Rx]]) = (uint32_t) pqp_regs[i.Ry];
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_MOVMR(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};
                    uint8_t gen_code[16];
                    size_t size = JIT::emmit_MOV<JIT::MOV_TYPES::M_R>(i, gen_code);
                    std::string i_text = JIT::to_string(gen_code, size);
                    std::cout << "MOV_M_R: " << i_text << std::endl;
                }
                break;
            }
            case CMP: {
                if (pqp_regs[i.Rx] < pqp_regs[i.Ry]) pqp_flags = 0b001;
                else if (pqp_regs[i.Rx] > pqp_regs[i.Ry]) pqp_flags = 0b010;
                else pqp_flags = 0b100;

                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_CMP(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};

                    uint8_t gen_code[16];
                    size_t size = JIT::emmit_BINARY_OP<PQP::CMP>(i, gen_code);
                    std::string i_text = JIT::to_string(gen_code, size);
                    std::cout << "CMP: " << i_text << std::endl;
                }
                break;
            }
            case JMP: {
                uint32_t tmp = PC;
                uint32_t target = tmp + (int32_t)i.i16 + 4;
                PC += (int32_t) i.i16;
                if (mapped_instructions.count(tmp)) {
                    mapped_instructions[tmp].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_JMP(++pqp_addr_counter[tmp], tmp, target);
                    mapped_instructions[tmp] = {1, s.str()};
                }
                break;
            }
            case JG: {
                uint32_t tmp = PC;
                uint32_t target = tmp + (int32_t)i.i16 + 4;
                if (pqp_flags & 0b010) PC += (int32_t) i.i16;
                if (mapped_instructions.count(tmp)) {
                    mapped_instructions[tmp].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_JG(++pqp_addr_counter[tmp], tmp, target);
                    mapped_instructions[tmp] = {1, s.str()};
                }
                break;
            }
            case JL: {
                uint32_t tmp = PC;
                uint32_t target = tmp + (int32_t)i.i16 + 4;
                if (pqp_flags & 0b001) PC += (int32_t) i.i16;
                if (mapped_instructions.count(tmp)) {
                    mapped_instructions[tmp].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_JL(++pqp_addr_counter[tmp], tmp, target);
                    mapped_instructions[tmp] = {1, s.str()};
                }
                break;
            }
            case JE: {
                uint32_t tmp = PC;
                uint32_t target = tmp + (int32_t)i.i16 + 4;
                if (pqp_flags & 0b100) PC += (int32_t) i.i16;
                if (mapped_instructions.count(tmp)) {
                    mapped_instructions[tmp].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_JE(++pqp_addr_counter[tmp], tmp, target);
                    mapped_instructions[tmp] = {1, s.str()};
                }
                break;
            }
            case ADD: {
                pqp_regs[i.Rx] += pqp_regs[i.Ry];
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_ADD(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};

                    uint8_t gen_code[16];
                    size_t size = JIT::emmit_BINARY_OP<PQP::ADD>(i, gen_code);
                    std::string i_text = JIT::to_string(gen_code, size);
                    std::cout << "ADD: " << i_text << std::endl;
                }
                break;
            }
            case SUB: {
                pqp_regs[i.Rx] -= pqp_regs[i.Ry];
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_SUB(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};

                    uint8_t gen_code[16];
                    size_t size = JIT::emmit_BINARY_OP<PQP::SUB>(i, gen_code);
                    std::string i_text = JIT::to_string(gen_code, size);
                    std::cout << "SUB: " << i_text << std::endl;
                }
                break;
            }
            case AND: {
                pqp_regs[i.Rx] &= pqp_regs[i.Ry];
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_AND(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};

                    uint8_t gen_code[16];
                    size_t size = JIT::emmit_BINARY_OP<PQP::AND>(i, gen_code);
                    std::string i_text = JIT::to_string(gen_code, size);
                    std::cout << "AND: " << i_text << std::endl;
                }
                break;
            }
            case OR: {
                pqp_regs[i.Rx] |= pqp_regs[i.Ry];
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_OR(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};

                    uint8_t gen_code[16];
                    size_t size = JIT::emmit_BINARY_OP<PQP::OR>(i, gen_code);
                    std::string i_text = JIT::to_string(gen_code, size);
                    std::cout << "OR: " << i_text << std::endl;
                }
                break;
            }
            case XOR: {
                pqp_regs[i.Rx] ^= pqp_regs[i.Ry];
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_XOR(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};

                    uint8_t gen_code[16];
                    size_t size = JIT::emmit_BINARY_OP<PQP::XOR>(i, gen_code);
                    std::string i_text = JIT::to_string(gen_code, size);
                    std::cout << "XOR: " << i_text << std::endl;
                }
                break;
            }
            case SAL: {
                pqp_regs[i.Rx] <<= i.i5;
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_SAL(++pqp_addr_counter[PC], PC, i.Rx, i.i5);
                    mapped_instructions[PC] = {1, s.str()};

                    uint8_t gen_code[16];
                    size_t size = JIT::emmit_SHIFT_OP<PQP::SAL>(i, gen_code);
                    std::string i_text = JIT::to_string(gen_code, size);
                    std::cout << "SAL: " << i_text << std::endl;
                }
                break;
            }
            case SAR:{
                pqp_regs[i.Rx] >>= i.i5;
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    std::stringstream s;
                    s << PRINT_SAR(++pqp_addr_counter[PC], PC, i.Rx, i.i5);
                    mapped_instructions[PC] = {1, s.str()};

                    uint8_t gen_code[16];
                    size_t size = JIT::emmit_SHIFT_OP<PQP::SAR>(i, gen_code);
                    std::string i_text = JIT::to_string(gen_code, size);
                    std::cout << "SAR: " << i_text << std::endl;
                }
                break;
            }
        
        }

        PC += 4;

    }

    mapped_instructions[PC] = {1, ""};

}

int main (int argc, char* argv[]){
    
    memset(PQP::pqp_mem, 0, 256);
    memset(PQP::pqp_regs, 0, 16 * sizeof(int));

    if (argc > 1) input_file = std::ifstream (argv[1]);
    if (argc > 2) output_file = std::ofstream (argv[2]);

    PQP::load_memory(input_file);

    interpreter();

    for (auto& [addr, counter] : PQP::mapped_instructions){
        if (addr < 256) output_file << LINE_HEADER(counter.first, addr) << counter.second << std::endl;
        else{
            output_file << "       -:" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)addr << ":exit" << std::endl;
        }
        
    }

    output_file << '\n';

    for (int j = 0; j < 15; j++){
           output_file << PRINT_REG(j) << PQP::pqp_regs[j] << ',';
       }

       output_file << PRINT_REG(15) << PQP::pqp_regs[15] << std::endl;

    return 0;
}