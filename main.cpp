/**
 * @file main.cpp
 * @brief Main entry point for the 6502 emulator
 * 
 * This file demonstrates how to use the 6502 emulator.
 * It sets up memory, loads a simple program, and executes it.
 */

#include "CPU.h"
#include "Memory.h"
#include "Constants.h"
#include <iostream>
#include <iomanip>

using namespace M6502;

/**
 * @brief Print current CPU state for debugging
 */
void PrintCPUState(const CPU& cpu) {
    std::cout << "╔═══════════════════════════════════════════╗\n";
    std::cout << "║         6502 CPU STATE                    ║\n";
    std::cout << "╠═══════════════════════════════════════════╣\n";
    std::cout << "║ PC: $" << std::hex << std::uppercase << std::setfill('0') 
              << std::setw(4) << static_cast<int>(cpu.PC) << "  ";
    std::cout << "SP: $" << std::setw(2) << static_cast<int>(cpu.SP) << "           ║\n";
    std::cout << "║ A:  $" << std::setw(2) << static_cast<int>(cpu.A) << "    ";
    std::cout << "X:  $" << std::setw(2) << static_cast<int>(cpu.X) << "    ";
    std::cout << "Y:  $" << std::setw(2) << static_cast<int>(cpu.Y) << "   ║\n";
    std::cout << "╠═══════════════════════════════════════════╣\n";
    std::cout << "║ Flags: ";
    std::cout << (cpu.P & FLAG_NEGATIVE  ? 'N' : '-');
    std::cout << (cpu.P & FLAG_OVERFLOW  ? 'V' : '-');
    std::cout << (cpu.P & FLAG_UNUSED    ? '1' : '0');
    std::cout << (cpu.P & FLAG_BREAK     ? 'B' : '-');
    std::cout << (cpu.P & FLAG_DECIMAL   ? 'D' : '-');
    std::cout << (cpu.P & FLAG_INTERRUPT ? 'I' : '-');
    std::cout << (cpu.P & FLAG_ZERO      ? 'Z' : '-');
    std::cout << (cpu.P & FLAG_CARRY     ? 'C' : '-');
    std::cout << "                   ║\n";
    std::cout << "╠═══════════════════════════════════════════╣\n";
    std::cout << "║ Total Cycles: " << std::dec << std::setw(10) << cpu.TotalCycles << "              ║\n";
    std::cout << "╚═══════════════════════════════════════════╝\n";
}

/**
 * @brief Example 1: Simple LDA/STA test
 */
void Example_LoadStoreTest() {
    std::cout << "\n═══════════════════════════════════════════\n";
    std::cout << "  Example 1: Load and Store Test\n";
    std::cout << "═══════════════════════════════════════════\n\n";
    
    CPU cpu;
    Memory memory;
    
    // Reset vector points to $1000
    memory[VECTOR_RESET] = 0x00;
    memory[VECTOR_RESET + 1] = 0x10;
    
    // Simple program at $1000:
    // LDA #$42    ; Load $42 into A
    // STA $0200   ; Store A at $0200
    // LDA $0200   ; Load from $0200 back into A
    // LDX #$FF    ; Load $FF into X
    // LDY #$0E    ; Load $0E into Y
    memory[0x1000] = INS_LDA_IM;
    memory[0x1001] = 0x42;
    memory[0x1002] = INS_STA_ABS;
    memory[0x1003] = 0x00;
    memory[0x1004] = 0x02;
    memory[0x1005] = INS_LDA_ABS;
    memory[0x1006] = 0x00;
    memory[0x1007] = 0x02;
    memory[0x1008] = INS_LDX_IM;
    memory[0x1009] = 0xFF;
    memory[0x100A] = INS_LDY_IM;
    memory[0x100B] = 0x0E;
    memory[0x100C] = INS_NOP;  // End with NOP
    
    // Reset CPU
    cpu.Reset(memory);
    std::cout << "After Reset:\n";
    PrintCPUState(cpu);
    
    // Execute instructions
    for (int i = 0; i < 7; i++) {
        cpu.Execute(memory);
        std::cout << "\nAfter instruction " << (i + 1) << ":\n";
        PrintCPUState(cpu);
    }
    
    std::cout << "\nMemory at $0200: $" << std::hex << std::uppercase 
              << static_cast<int>(memory[0x0200]) << "\n";
}

/**
 * @brief Example 2: Arithmetic test with ADC
 */
void Example_ArithmeticTest() {
    std::cout << "\n═══════════════════════════════════════════\n";
    std::cout << "  Example 2: Arithmetic Test (ADC)\n";
    std::cout << "═══════════════════════════════════════════\n\n";
    
    CPU cpu;
    Memory memory;
    
    // Reset vector points to $1000
    memory[VECTOR_RESET] = 0x00;
    memory[VECTOR_RESET + 1] = 0x10;
    
    // Program: Add 5 + 3 = 8
    // CLC         ; Clear carry
    // LDA #$05    ; A = 5
    // ADC #$03    ; A = A + 3 = 8
    memory[0x1000] = INS_CLC;
    memory[0x1001] = INS_LDA_IM;
    memory[0x1002] = 0x05;
    memory[0x1003] = INS_ADC_IM;
    memory[0x1004] = 0x03;
    memory[0x1005] = INS_NOP;
    
    cpu.Reset(memory);
    std::cout << "After Reset:\n";
    PrintCPUState(cpu);
    
    // Execute instructions
    for (int i = 0; i < 4; i++) {
        cpu.Execute(memory);
        std::cout << "\nAfter instruction " << (i + 1) << ":\n";
        PrintCPUState(cpu);
    }
    
    std::cout << "\nResult: A = $" << std::hex << std::uppercase 
              << static_cast<int>(cpu.A) << " (should be $08)\n";
}

/**
 * @brief Example 3: Loop with branch
 */
void Example_LoopTest() {
    std::cout << "\n═══════════════════════════════════════════\n";
    std::cout << "  Example 3: Loop Test (Count to 5)\n";
    std::cout << "═══════════════════════════════════════════\n\n";
    
    CPU cpu;
    Memory memory;
    
    // Reset vector points to $1000
    memory[VECTOR_RESET] = 0x00;
    memory[VECTOR_RESET + 1] = 0x10;
    
    // Program: Count from 0 to 5 in X register
    // LDX #$00    ; X = 0        $1000
    // Loop:
    // INX         ; X++          $1002
    // CPX #$05    ; Compare X to 5  $1003
    // BNE Loop    ; Branch if X != 5  $1005
    // NOP                        $1007
    memory[0x1000] = INS_LDX_IM;
    memory[0x1001] = 0x00;
    memory[0x1002] = INS_INX;
    memory[0x1003] = INS_CPX_IM;
    memory[0x1004] = 0x05;
    memory[0x1005] = INS_BNE;
    memory[0x1006] = 0xFA;  // -6 (jump back to $1002)
    memory[0x1007] = INS_NOP;
    
    cpu.Reset(memory);
    std::cout << "After Reset:\n";
    PrintCPUState(cpu);
    
    // Execute until NOP at end (or max iterations for safety)
    int maxIterations = 50;
    int iteration = 0;
    
    while (cpu.PC != 0x1007 && iteration < maxIterations) {
        cpu.Execute(memory);
        iteration++;
    }
    
    cpu.Execute(memory);  // Execute final NOP
    
    std::cout << "\nAfter loop completion:\n";
    PrintCPUState(cpu);
    std::cout << "\nResult: X = $" << std::hex << std::uppercase 
              << static_cast<int>(cpu.X) << " (should be $05)\n";
    std::cout << "Iterations: " << std::dec << iteration << "\n";
}

/**
 * @brief Main entry point
 */
int main() {
    std::cout << "╔═══════════════════════════════════════════╗\n";
    std::cout << "║   6502 Microprocessor Emulator            ║\n";
    std::cout << "║   Rockwell R650X/R651X Compatible         ║\n";
    std::cout << "║   Version 1.0.0                           ║\n";
    std::cout << "╚═══════════════════════════════════════════╝\n";
    
    try {
        // Run examples
        Example_LoadStoreTest();
        Example_ArithmeticTest();
        Example_LoopTest();
        
        std::cout << "\n═══════════════════════════════════════════\n";
        std::cout << "  All examples completed successfully!\n";
        std::cout << "═══════════════════════════════════════════\n\n";
        
        std::cout << "The emulator implements the complete 6502 instruction set:\n";
        std::cout << "  ✓ Load/Store operations (LDA, LDX, LDY, STA, STX, STY)\n";
        std::cout << "  ✓ Register transfers (TAX, TAY, TXA, TYA, TSX, TXS)\n";
        std::cout << "  ✓ Stack operations (PHA, PHP, PLA, PLP)\n";
        std::cout << "  ✓ Logical operations (AND, ORA, EOR, BIT)\n";
        std::cout << "  ✓ Arithmetic (ADC, SBC with BCD support)\n";
        std::cout << "  ✓ Increment/Decrement (INC, INX, INY, DEC, DEX, DEY)\n";
        std::cout << "  ✓ Shifts/Rotates (ASL, LSR, ROL, ROR)\n";
        std::cout << "  ✓ Jumps/Branches (JMP, JSR, RTS, RTI, Bxx)\n";
        std::cout << "  ✓ Comparisons (CMP, CPX, CPY)\n";
        std::cout << "  ✓ Flag operations (CLC, SEC, CLI, SEI, etc.)\n";
        std::cout << "  ✓ System (BRK, NOP)\n";
        std::cout << "  ✓ Cycle-accurate execution\n";
        std::cout << "  ✓ All addressing modes\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
