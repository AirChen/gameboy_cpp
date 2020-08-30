#ifndef CPU_1
#define CPU_1

#include <vector>
#include <chrono>
class Memory;

typedef enum {
    Term_GB,  // Original GameBoy (GameBoy Classic)
    Term_GBP, // GameBoy Pocket/GameBoy Light
    Term_GBC, // GameBoy Color
    Term_SGB, // Super GameBoy
} Term;

typedef enum {
    // Zero Flag. This bit is set when the result of a math operationis zero or two values match when using the CP
    // instruction.
    Flag_Z = 0b10000000,
    // Subtract Flag. This bit is set if a subtraction was performed in the last math instruction.
    Flag_N = 0b01000000,
    // Half Carry Flag. This bit is set if a carry occurred from the lowernibble in the last math operation.
    Flag_H = 0b00100000,
    // Carry Flag. This bit is set if a carry occurred from the last math operation or if register A is the smaller
    // valuewhen executing the CP instruction.
    Flag_C = 0b00010000,
} Flag;

class Register
{
public:
    uint8_t a;    
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t f; // The F register is indirectly accessible by the programer.
    uint8_t h;
    uint8_t l;
    uint16_t sp;
    uint16_t pc;

public:
    Register(Term term);    

    uint16_t get_af();

    uint16_t get_bc();    

    uint16_t get_de();    

    uint16_t get_hl();    

    void set_af(uint16_t v);    

    void set_bc(uint16_t v);    

    void set_de(uint16_t v);    

    void set_hl(uint16_t v);    

    bool get_flag(Flag fl);    

    void set_flag(Flag fl, bool v);    
};

class Cpu
{
public:        
    Register *reg;      // 寄存器
    Memory *mem;        // 可访问的内存空间
    bool halted;        // 表明 CPU 是否处于工作状态
    bool ei;            // enable interrupt 的简写, 表明 CPU 是否接收硬件中断

public:
    Cpu(Term term, Memory *m);
    ~Cpu();

    uint32_t hi();
    uint32_t ex();
    uint32_t next();

    uint8_t imm();
    uint16_t imm_word();

    void stack_add(uint16_t v);
    uint16_t stack_pop();

    // Add n to A.
    // n = A,B,C,D,E,H,L,(HL),#
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Reset.
    // H - Set if carry from bit 3.
    // C - Set if carry from bit 7.
    void alu_add(uint8_t n);

    // Add n + Carry flag to A.
    // n = A,B,C,D,E,H,L,(HL),#
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Reset.
    // H - Set if carry from bit 3.
    // C - Set if carry from bit 7.
    void alu_adc(uint8_t n);

    // Subtract n from A.
    // n = A,B,C,D,E,H,L,(HL),#
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Set.
    // H - Set if no borrow from bit 4.
    // C - Set if no borrow
    void alu_sub(uint8_t n);

    // Subtract n + Carry flag from A.
    // n = A,B,C,D,E,H,L,(HL),#
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Set.
    // H - Set if no borrow from bit 4.
    // C - Set if no borrow.
    void alu_sbc(uint8_t n);

    // Logically AND n with A, result in A.
    // n = A,B,C,D,E,H,L,(HL),#
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Reset.
    // H - Set.
    // C - Reset
    void alu_and(uint8_t n);

    // Logical OR n with register A, result in A.
    // n = A,B,C,D,E,H,L,(HL),#
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Reset.
    // H - Reset.
    // C - Reset.
    void alu_or(uint8_t n);

    // Logical exclusive OR n with register A, result in A.
    // n = A,B,C,D,E,H,L,(HL),#
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Reset.
    // H - Reset.
    // C - Reset.
    void alu_xor(uint8_t n);

    // Compare A with n. This is basically an A - n subtraction instruction but the results are thrown away.
    // n = A,B,C,D,E,H,L,(HL),#
    //
    // Flags affected:
    // Z - Set if result is zero. (Set if A = n.)
    // N - Set.
    // H - Set if no borrow from bit 4.
    // C - Set for no borrow. (Set if A < n.)
    void alu_cp(uint8_t n);

    // Increment register n.
    // n = A,B,C,D,E,H,L,(HL)
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Reset.
    // H - Set if carry from bit 3.
    // C - Not affected.
    uint8_t alu_inc(uint8_t a);

    // Decrement register n.
    // n = A,B,C,D,E,H,L,(HL)
    //
    // Flags affected:
    // Z - Set if reselt is zero.
    // N - Set.
    // H - Set if no borrow from bit 4.
    // C - Not affected
    uint8_t alu_dec(uint8_t a);

    // Add n to HL
    // n = BC,DE,HL,SP
    //
    // Flags affected:
    // Z - Not affected.
    // N - Reset.
    // H - Set if carry from bit 11.
    // C - Set if carry from bit 15.
    void alu_add_hl(uint16_t n);

    // Add n to Stack Pointer (SP).
    // n = one byte signed immediate value (#).
    //
    // Flags affected:
    // Z - Reset.
    // N - Reset.
    // H - Set or reset according to operation.
    // C - Set or reset according to operation.
    void alu_add_sp();

    // Swap upper & lower nibles of n.
    // n = A,B,C,D,E,H,L,(HL)
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Reset.
    // H - Reset.
    // C - Reset.
    uint8_t alu_swap(uint8_t a);

    // Decimal adjust register A. This instruction adjusts register A so that the correct representation of Binary
    // Coded Decimal (BCD) is obtained.
    //
    // Flags affected:
    // Z - Set if register A is zero.
    // N - Not affected.
    // H - Reset.
    // C - Set or reset according to operation
    void alu_daa();

    // Complement A register. (Flip all bits.)
    //
    // Flags affected:
    // Z - Not affected.
    // N - Set.
    // H - Set.
    // C - Not affected.
    void alu_cpl();

    // Complement carry flag. If C flag is set, then reset it. If C flag is reset, then set it.
    // Flags affected:
    //
    // Z - Not affected.
    // N - Reset.
    // H - Reset.
    // C - Complemented.
    void alu_ccf();

    // Set Carry flag.
    //
    // Flags affected:
    // Z - Not affected.
    // N - Reset.
    // H - Reset.
    // C - Set.
    void alu_scf();

    // Rotate A left. Old bit 7 to Carry flag.
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Reset.
    // H - Reset.
    // C - Contains old bit 7 data.
    uint8_t alu_rlc(uint8_t a);

    // Rotate A left through Carry flag.
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Reset.
    // H - Reset.
    // C - Contains old bit 7 data.
    uint8_t alu_rl(uint8_t a);

    // Rotate A right. Old bit 0 to Carry flag.
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Reset.
    // H - Reset.
    // C - Contains old bit 0 data
    uint8_t alu_rrc(uint8_t a);

    // Rotate A right through Carry flag.
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Reset.
    // H - Reset.
    // C - Contains old bit 0 data.
    uint8_t alu_rr(uint8_t a);

    // Shift n left into Carry. LSB of n set to 0.
    // n = A,B,C,D,E,H,L,(HL)
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Reset.
    // H - Reset.
    // C - Contains old bit 7 data
    uint8_t alu_sla(uint8_t a);

    // Shift n right into Carry. MSB doesn't change.
    // n = A,B,C,D,E,H,L,(HL)
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Reset.
    // H - Reset.
    // C - Contains old bit 0 data.
    uint8_t alu_sra(uint8_t a);

    // Shift n right into Carry. MSB set to 0.
    // n = A,B,C,D,E,H,L,(HL)
    //
    // Flags affected:
    // Z - Set if result is zero.
    // N - Reset.
    // H - Reset.
    // C - Contains old bit 0 data.
    uint8_t alu_srl(uint8_t a);

    // Test bit b in register r.
    // b = 0 - 7, r = A,B,C,D,E,H,L,(HL)
    //
    // Flags affected:
    // Z - Set if bit b of register r is 0.
    // N - Reset.
    // H - Set.
    // C - Not affected
    void alu_bit(uint8_t a, uint8_t b);

    // Set bit b in register r.
    // b = 0 - 7, r = A,B,C,D,E,H,L,(HL)
    //
    // Flags affected:  None.
    uint8_t alu_set(uint8_t a, uint8_t b);

    // Reset bit b in register r.
    // b = 0 - 7, r = A,B,C,D,E,H,L,(HL)
    //
    // Flags affected:  None.
    uint8_t alu_res(uint8_t a, uint8_t b);

    // Add n to current address and jump to it.
    // n = one byte signed immediate value
    void alu_jr(uint8_t n);
};

class Rtc
{
private:
    uint32_t step_cycles;
    bool step_flip;    
    std::chrono::steady_clock::time_point step_zero;    
public:
    Cpu *cpu;
    Rtc(Term term, Memory *m);
    ~Rtc();

    // Function next simulates real hardware execution speed, by limiting the frequency of the function cpu.next().
    uint32_t next();
    bool flip();
};

#endif