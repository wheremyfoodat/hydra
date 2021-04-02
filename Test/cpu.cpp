#include "cpu.h"
#include <stdexcept>
#include <iostream>

CPU::CPU() {
	A = 0; B = 0; C = 0; D = 0; E = 0; H = 0; L = 0;
	F = 0; SP = 0; PC = 0x0; IME = 1; R = 0;
	mClock = 0; tClock = 0;
	halt = false; stop = false;

	instructions =
	{
		{ "NOP" , &CPU::NOP }, { "LDBC16" , &CPU::LDBC16 , 2}, { "LDBCA" , &CPU::LDBCA }, { "INCBC" , &CPU::INCBC }, { "INCB" , &CPU::INCB }, { "DECB" , &CPU::DECB }, { "LDB8" , &CPU::LDB8 , 1}, { "RLCA" , &CPU::RLCA }, { "LD16SP" , &CPU::LD16SP }, { "ADDHLBC" , &CPU::ADDHLBC }, { "LDABC" , &CPU::LDABC }, { "DECBC" , &CPU::DECBC }, { "INCC" , &CPU::INCC }, { "DECC" , &CPU::DECC }, { "LDC8" , &CPU::LDC8 , 1}, { "RRCA" , &CPU::RRCA },
		{ "STOP" , &CPU::STOP }, { "LDDE16" , &CPU::LDDE16 , 2}, { "LDDEA" , &CPU::LDDEA }, { "INCDE" , &CPU::INCDE }, { "INCD" , &CPU::INCD }, { "DECD" , &CPU::DECD }, { "LDD8" , &CPU::LDD8 , 1}, { "RLA" , &CPU::RLA }, { "JR8" , &CPU::JR8 , 1}, { "ADDHLDE" , &CPU::ADDHLDE }, { "LDADE" , &CPU::LDADE }, { "DECDE" , &CPU::DECDE }, { "INCE" , &CPU::INCE }, { "DECE" , &CPU::DECE }, { "LDE8" , &CPU::LDE8 , 1}, { "RRA" , &CPU::RRA },
		{ "JRNZ8" , &CPU::JRNZ8 , 1}, { "LDHL16" , &CPU::LDHL16 , 2}, { "LDIHLA" , &CPU::LDIHLA }, { "INCHL" , &CPU::INCHL }, { "INCH" , &CPU::INCH }, { "DECH" , &CPU::DECH }, { "LDH8" , &CPU::LDH8 , 1}, { "???" , &CPU::XXX }, { "JRZ8" , &CPU::JRZ8 , 1}, { "ADDHLHL" , &CPU::ADDHLHL }, { "LDIAHL" , &CPU::LDIAHL }, { "DECHL" , &CPU::DECHL }, { "INCL" , &CPU::INCL }, { "DECL" , &CPU::DECL }, { "LDL8" , &CPU::LDL8 , 1}, { "CPL" , &CPU::CPL },
		{ "JRNC8" , &CPU::JRNC8 , 1}, { "LDSP16" , &CPU::LDSP16 , 2}, { "LDDHLA" , &CPU::LDDHLA }, { "INCSP" , &CPU::INCSP }, { "INCHLR" , &CPU::INCHLR }, { "DECHLR" , &CPU::DECHLR }, { "LDHL8" , &CPU::LDHL8 , 1}, { "SCF" , &CPU::SCF }, { "JRC8" , &CPU::JRC8 , 1}, { "ADDHLSP" , &CPU::ADDHLSP }, { "LDDAHL" , &CPU::LDDAHL }, { "DECSP" , &CPU::DECSP }, { "INCA" , &CPU::INCA }, { "DECA" , &CPU::DECA }, { "LDA8" , &CPU::LDA8 , 1}, { "CCF" , &CPU::CCF },
		{ "LDBB" , &CPU::LDBB }, { "LDBC" , &CPU::LDBC }, { "LDBD" , &CPU::LDBD }, { "LDBE" , &CPU::LDBE }, { "LDBH" , &CPU::LDBH }, { "LDBL" , &CPU::LDBL }, { "LDBHL" , &CPU::LDBHL }, { "LDBA" , &CPU::LDBA }, { "LDCB" , &CPU::LDCB }, { "LDCC" , &CPU::LDCC }, { "LDCD" , &CPU::LDCD }, { "LDCE" , &CPU::LDCE }, { "LDCH" , &CPU::LDCH }, { "LDCL" , &CPU::LDCL }, { "LDCHL" , &CPU::LDCHL }, { "LDCA" , &CPU::LDCA },
		{ "LDDB" , &CPU::LDDB }, { "LDDC" , &CPU::LDDC }, { "LDDD" , &CPU::LDDD }, { "LDDE" , &CPU::LDDE }, { "LDDH" , &CPU::LDDH }, { "LDDL" , &CPU::LDDL }, { "LDDHL" , &CPU::LDDHL }, { "LDDA" , &CPU::LDDA }, { "LDEB" , &CPU::LDEB }, { "LDEC" , &CPU::LDEC }, { "LDED" , &CPU::LDED }, { "LDEE" , &CPU::LDEE }, { "LDEH" , &CPU::LDEH }, { "LDEL" , &CPU::LDEL }, { "LDEHL" , &CPU::LDEHL }, { "LDEA" , &CPU::LDEA },
		{ "LDHB" , &CPU::LDHB }, { "LDHC" , &CPU::LDHC }, { "LDHD" , &CPU::LDHD }, { "LDHE" , &CPU::LDHE }, { "LDHH" , &CPU::LDHH }, { "LDHL" , &CPU::LDHL }, { "LDHHL" , &CPU::LDHHL }, { "LDHA" , &CPU::LDHA }, { "LDLB" , &CPU::LDLB }, { "LDLC" , &CPU::LDLC }, { "LDLD" , &CPU::LDLD }, { "LDLE" , &CPU::LDLE }, { "LDLH" , &CPU::LDLH }, { "LDLL" , &CPU::LDLL }, { "LDLHL" , &CPU::LDLHL }, { "LDLA" , &CPU::LDLA },
		{ "LDHLB" , &CPU::LDHLB }, { "LDHLC" , &CPU::LDHLC }, { "LDHLD" , &CPU::LDHLD }, { "LDHLE" , &CPU::LDHLE }, { "LDHLH" , &CPU::LDHLH }, { "LDHLL" , &CPU::LDHLL }, { "HALT" , &CPU::HALT }, { "LDHLA" , &CPU::LDHLA }, { "LDAB" , &CPU::LDAB }, { "LDAC" , &CPU::LDAC }, { "LDAD" , &CPU::LDAD }, { "LDAE" , &CPU::LDAE }, { "LDAH" , &CPU::LDAH }, { "LDAL" , &CPU::LDAL }, { "LDAHL" , &CPU::LDAHL }, { "LDAA" , &CPU::LDAA },
		{ "ADDAB" , &CPU::ADDAB }, { "ADDAC" , &CPU::ADDAC }, { "ADDAD" , &CPU::ADDAD }, { "ADDAE" , &CPU::ADDAE }, { "ADDAH" , &CPU::ADDAH }, { "ADDAL" , &CPU::ADDAL }, { "ADDAHL" , &CPU::ADDAHL }, { "ADDAA" , &CPU::ADDAA }, { "ADCAB" , &CPU::ADCAB }, { "ADCAC" , &CPU::ADCAC }, { "ADCAD" , &CPU::ADCAD }, { "ADCAE" , &CPU::ADCAE }, { "ADCAH" , &CPU::ADCAH }, { "ADCAL" , &CPU::ADCAL }, { "ADCAHL" , &CPU::ADCAHL }, { "ADCAA" , &CPU::ADCAA },
		{ "SUBAB" , &CPU::SUBAB }, { "SUBAC" , &CPU::SUBAC }, { "SUBAD" , &CPU::SUBAD }, { "SUBAE" , &CPU::SUBAE }, { "SUBAH" , &CPU::SUBAH }, { "SUBAL" , &CPU::SUBAL }, { "SUBAHL" , &CPU::SUBAHL }, { "SUBAA" , &CPU::SUBAA }, { "SBCAB" , &CPU::SBCAB }, { "SBCAC" , &CPU::SBCAC }, { "SBCAD" , &CPU::SBCAD }, { "SBCAE" , &CPU::SBCAE }, { "SBCAH" , &CPU::SBCAH }, { "SBCAL" , &CPU::SBCAL }, { "SBCAHL" , &CPU::SBCAHL }, { "SBCAA" , &CPU::SBCAA },
		{ "ANDB" , &CPU::ANDB }, { "ANDC" , &CPU::ANDC }, { "ANDD" , &CPU::ANDD }, { "ANDE" , &CPU::ANDE }, { "ANDH" , &CPU::ANDH }, { "ANDL" , &CPU::ANDL }, { "ANDHL" , &CPU::ANDHL }, { "ANDA" , &CPU::ANDA }, { "XORB" , &CPU::XORB }, { "XORC" , &CPU::XORC }, { "XORD" , &CPU::XORD }, { "XORE" , &CPU::XORE }, { "XORH" , &CPU::XORH }, { "XORL" , &CPU::XORL }, { "XORHL" , &CPU::XORHL }, { "XORA" , &CPU::XORA },
		{ "ORB" , &CPU::ORB }, { "ORC" , &CPU::ORC }, { "ORD" , &CPU::ORD }, { "ORE" , &CPU::ORE }, { "ORH" , &CPU::ORH }, { "ORL" , &CPU::ORL }, { "ORHL" , &CPU::ORHL }, { "ORA" , &CPU::ORA }, { "CPAB" , &CPU::CPAB }, { "CPAC" , &CPU::CPAC }, { "CPAD" , &CPU::CPAD }, { "CPAE" , &CPU::CPAE }, { "CPAH" , &CPU::CPAH }, { "CPAL" , &CPU::CPAL }, { "CPAHL" , &CPU::CPAHL }, { "CPAA" , &CPU::CPAA },
		{ "RETNZ" , &CPU::RETNZ }, { "POPBC" , &CPU::POPBC }, { "JPNZ16" , &CPU::JPNZ16 , 2}, { "JP16" , &CPU::JP16 , 2}, { "CALLNZ16" , &CPU::CALLNZ16 , 2}, { "PUSHBC" , &CPU::PUSHBC }, { "ADDA8" , &CPU::ADDA8 , 1}, { "RST0" , &CPU::RST0 }, { "RETZ" , &CPU::RETZ }, { "RET" , &CPU::RET }, { "JPZ16" , &CPU::JPZ16 , 2}, { "EXT" , &CPU::EXT , 1 }, { "CALLZ16" , &CPU::CALLZ16 , 2}, { "CALL16" , &CPU::CALL16 , 2}, { "ADCA16" , &CPU::ADCA16 , 2}, { "RST8" , &CPU::RST8 },
		{ "RETNC" , &CPU::RETNC }, { "POPDE" , &CPU::POPDE }, { "JPNC16" , &CPU::JPNC16 , 2}, { "???" , &CPU::XXX }, { "CALLNC16" , &CPU::CALLNC16 , 2}, { "PUSHDE" , &CPU::PUSHDE }, { "SUBA8" , &CPU::SUBA8 , 1}, { "RST10" , &CPU::RST10 }, { "RETC" , &CPU::RETC }, { "RETI" , &CPU::RETI }, { "JPC16" , &CPU::JPC16 , 2}, { "???" , &CPU::XXX }, { "CALLC16" , &CPU::CALLC16 , 2}, { "???" , &CPU::XXX }, { "SBCA8" , &CPU::SBCA8 , 1}, { "RST18" , &CPU::RST18 },
		{ "LDH8A" , &CPU::LDH8A }, { "POPHL" , &CPU::POPHL }, { "LDHCA" , &CPU::LDHCA }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "PUSHHL" , &CPU::PUSHHL }, { "AND8" , &CPU::AND8 , 1}, { "RST20" , &CPU::RST20 }, { "ADDSPD" , &CPU::ADDSPD , 1}, { "JPHL" , &CPU::JPHL }, { "LD16A" , &CPU::LD16A }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "XOR8" , &CPU::XOR8 , 1}, { "RST28" , &CPU::RST28 },
		{ "LDHA8" , &CPU::LDHA8 , 1}, { "POPAF" , &CPU::POPAF }, { "???" , &CPU::XXX }, { "DI" , &CPU::DI }, { "???" , &CPU::XXX }, { "PUSHAF" , &CPU::PUSHAF }, { "OR8" , &CPU::OR8 , 1}, { "RST30" , &CPU::RST30 }, { "LDHLSPD" , &CPU::LDHLSPD , 1 }, { "???" , &CPU::XXX }, { "LDA16" , &CPU::LDA16}, { "EI" , &CPU::EI }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "CP8" , &CPU::CP8 , 1}, { "RST38" , &CPU::RST38 }
	};

	cbMap = {
		{ "RLCB" , &CPU::RLCB }, { "RLCC" , &CPU::RLCC }, { "RLCD" , &CPU::RLCD }, { "RLCE" , &CPU::RLCE }, { "RLCH" , &CPU::RLCH }, { "RLCL" , &CPU::RLCL }, { "RLCHL" , &CPU::RLCHL }, { "RLCAr" , &CPU::RLCAr },  { "RRCB" , &CPU::RRCB }, { "RRCC" , &CPU::RRCC }, { "RRCD" , &CPU::RRCD }, { "RRCE" , &CPU::RRCE }, { "RRCH" , &CPU::RRCH }, { "RRCL" , &CPU::RRCL }, { "RRCHL" , &CPU::RRCHL }, { "RRCAr" , &CPU::RRCAr },
		{ "RLB" , &CPU::RLB }, { "RLC" , &CPU::RLC }, { "RLD" , &CPU::RLD }, { "RLE" , &CPU::RLE }, { "RLH" , &CPU::RLH }, { "RLL" , &CPU::RLL }, { "RLHL" , &CPU::RLHL }, { "RLAr" , &CPU::RLAr }, { "RRB" , &CPU::RRB }, { "RRC" , &CPU::RRC },  { "RRD" , &CPU::RRD },  { "RRE" , &CPU::RRE },  { "RRH" , &CPU::RRH },  { "RRL" , &CPU::RRL },  { "RRHL" , &CPU::RRHL },  { "RRAr" , &CPU::RRAr },
		{ "SLAB" , &CPU::SLAB }, { "SLAC" , &CPU::SLAC }, { "SLAD" , &CPU::SLAD }, { "SLAE" , &CPU::SLAE }, { "SLAH" , &CPU::SLAH }, { "SLAL" , &CPU::SLAL }, { "???" , &CPU::XXX }, { "SLAA" , &CPU::SLAA }, { "SRAB" , &CPU::SRAB }, { "SRAC" , &CPU::SRAC }, { "SRAD" , &CPU::SRAD }, { "SRAE" , &CPU::SRAE }, { "SRAH" , &CPU::SRAH }, { "SRAL" , &CPU::SRAL }, { "???" , &CPU::XXX }, { "SRAA" , &CPU::SRAA },
		{ "SWAPB" , &CPU::SWAPB }, { "SWAPC" , &CPU::SWAPC }, { "SWAPD" , &CPU::SWAPD }, { "SWAPE" , &CPU::SWAPE }, { "SWAPH" , &CPU::SWAPH }, { "SWAPL" , &CPU::SWAPL }, { "???" , &CPU::XXX }, { "SWAPA" , &CPU::SWAPA }, { "SRLB" , &CPU::SRLB }, { "SRLC" , &CPU::SRLC }, { "SRLD" , &CPU::SRLD }, { "SRLE" , &CPU::SRLE }, { "SRLH" , &CPU::SRLH }, { "SRLL" , &CPU::SRLL }, { "???" , &CPU::XXX }, { "SRLA" , &CPU::SRLA },
		{ "BIT0B" , &CPU::BIT0B }, { "BIT0C" , &CPU::BIT0C }, { "BIT0D" , &CPU::BIT0D }, { "BIT0E" , &CPU::BIT0E }, { "BIT0H" , &CPU::BIT0H }, { "BIT0L" , &CPU::BIT0L }, { "BIT0M" , &CPU::BIT0M }, { "BIT0A" , &CPU::BIT0A }, { "BIT1B" , &CPU::BIT1B }, { "BIT1C" , &CPU::BIT1C }, { "BIT1D" , &CPU::BIT1D }, { "BIT1E" , &CPU::BIT1E }, { "BIT1H" , &CPU::BIT1H }, { "BIT1L" , &CPU::BIT1L }, { "BIT1M" , &CPU::BIT1M }, { "BIT1A" , &CPU::BIT1A },
		{ "BIT2B" , &CPU::BIT2B }, { "BIT2C" , &CPU::BIT2C }, { "BIT2D" , &CPU::BIT2D }, { "BIT2E" , &CPU::BIT2E }, { "BIT2H" , &CPU::BIT2H }, { "BIT2L" , &CPU::BIT2L }, { "BIT2M" , &CPU::BIT2M }, { "BIT2A" , &CPU::BIT2A }, { "BIT3B" , &CPU::BIT3B }, { "BIT3C" , &CPU::BIT3C }, { "BIT3D" , &CPU::BIT3D }, { "BIT3E" , &CPU::BIT3E }, { "BIT3H" , &CPU::BIT3H }, { "BIT3L" , &CPU::BIT3L }, { "BIT3M" , &CPU::BIT3M }, { "BIT3A" , &CPU::BIT3A },
		{ "BIT4B" , &CPU::BIT4B }, { "BIT4C" , &CPU::BIT4C }, { "BIT4D" , &CPU::BIT4D }, { "BIT4E" , &CPU::BIT4E }, { "BIT4H" , &CPU::BIT4H }, { "BIT4L" , &CPU::BIT4L }, { "BIT4M" , &CPU::BIT4M }, { "BIT4A" , &CPU::BIT4A }, { "BIT5B" , &CPU::BIT5B }, { "BIT5C" , &CPU::BIT5C }, { "BIT5D" , &CPU::BIT5D }, { "BIT5E" , &CPU::BIT5E }, { "BIT5H" , &CPU::BIT5H }, { "BIT5L" , &CPU::BIT5L }, { "BIT5M" , &CPU::BIT5M }, { "BIT5A" , &CPU::BIT5A },
		{ "BIT6B" , &CPU::BIT6B }, { "BIT6C" , &CPU::BIT6C }, { "BIT6D" , &CPU::BIT6D }, { "BIT6E" , &CPU::BIT6E }, { "BIT6H" , &CPU::BIT6H }, { "BIT6L" , &CPU::BIT6L }, { "BIT6M" , &CPU::BIT6M }, { "BIT6A" , &CPU::BIT6A }, { "BIT7B" , &CPU::BIT7B }, { "BIT7C" , &CPU::BIT7C }, { "BIT7D" , &CPU::BIT7D }, { "BIT7E" , &CPU::BIT7E }, { "BIT7H" , &CPU::BIT7H }, { "BIT7L" , &CPU::BIT7L }, { "BIT7M" , &CPU::BIT7M }, { "BIT7A" , &CPU::BIT7A },
		{ "RES0B" , &CPU::XXX }, { "RES0C" , &CPU::XXX }, { "RES0D" , &CPU::XXX }, { "RES0E" , &CPU::XXX }, { "RES0H" , &CPU::XXX }, { "RES01" , &CPU::XXX }, { "RES08" , &CPU::XXX }, { "RES0A" , &CPU::RES0A }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX },
		{ "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX },
		{ "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX },
		{ "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX },
		{ "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX },
		{ "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX },
		{ "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX },
		{ "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX }, { "???" , &CPU::XXX },
	};

	cpuBus = std::shared_ptr<Bus>(new Bus());
	gpu = std::shared_ptr<GPU>(new GPU(cpuBus));
}

CPU::~CPU() {

}

CPU::Instruction CPU::GetInstruction(int index) {
	return instructions[index];
}

std::vector<CPU::Instruction> CPU::ConstructInstructionList(std::vector<int>& ints) {
	std::vector<CPU::Instruction> ret;
	int x1 = PC;
	int x2 = PC + 20;
	for (int i = x1; i < x2; i++) {
		Instruction temp = GetInstruction(cpuBus->mem[i]);
		temp.PC = i;
		if (temp.skip != 0) {
			i += temp.skip;
			temp.byte1 = i + 1;
			if (temp.skip == 2) {
				temp.byte2 = i + 2;
			}
		}
		ret.push_back(temp);
	}
	return ret;
}

void CPU::FZ(int i, bool as) {
	F = 0;
	if (!(i & 0xFF)) {
		F |= 0x80;
	}

	F |= as ? 0x40 : 0;
}

void CPU::RSV() {
	rsvA = A; rsvB = B;
	rsvC = C; rsvD = D;
	rsvE = E; rsvH = H;
	rsvL = L; rsvF = F;
}

void CPU::RRS() {
	A = rsvA; B = rsvB;
	C = rsvC; D = rsvD;
	E = rsvE; H = rsvH;
	L = rsvL; F = rsvF;
}

#pragma region Instructions

void CPU::ADDAA() {
	A += A;
	F = 0;
	if (!(A & 0xFF)) F |= 0x80;
	if (A > 0xFF) F |= 0x10;
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::ADDAB() {
	A += B;
	F = 0;
	if (!(A & 0xFF)) F |= 0x80;
	if (A > 0xFF) F |= 0x10;
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::ADDAC() {
	A += C;
	F = 0;
	if (!(A & 0xFF)) F |= 0x80;
	if (A > 0xFF) F |= 0x10;
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::ADDAD() {
	A += D;
	F = 0;
	if (!(A & 0xFF)) F |= 0x80;
	if (A > 0xFF) F |= 0x10;
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::ADDAE() {
	A += E;
	F = 0;
	if (!(A & 0xFF)) F |= 0x80;
	if (A > 0xFF) F |= 0x10;
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::ADDAH() {
	A += H;
	F = 0;
	if (!(A & 0xFF)) F |= 0x80;
	if (A > 0xFF) F |= 0x10;
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::ADDAL() {
	A += L;
	F = 0;
	if (!(A & 0xFF)) F |= 0x80;
	if (A > 0xFF) F |= 0x10;
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::ADDAHL() {
	A += cpuBus->Read((H << 8) | L);
	FZ(A);
	if (A > 0xFF)
		F |= 0x10;
	A &= 0xFF;
	mTemp = 2; tTemp = 8;
}

void CPU::ADDA8() {
	A += cpuBus->Read(PC);
	FZ(A);
	PC++;
	if (A > 0xFF)
		F |= 0x10;
	A &= 0xFF;
	mTemp = 2; tTemp = 8;
}

void CPU::ADDHLBC() {
	int HL = (H << 8) | L;
	HL += (B << 8) | C;
	if (HL > 0xFF) {
		F |= 0x10;
	}
	else {
		F &= 0xEF;
	}
	H = (HL >> 8) & 255;
	L = HL & 255;
	mTemp = 3; tTemp = 12;
}

void CPU::ADDHLDE() {
	int HL = (H << 8) | L;
	HL += (D << 8) | E;
	if (HL > 0xFF) {
		F |= 0x10;
	}
	else {
		F &= 0xEF;
	}
	H = (HL >> 8) & 255;
	L = HL & 255;
	mTemp = 3; tTemp = 12;
}

void CPU::ADDHLHL() {
	int HL = (H << 8) | L;
	HL += HL;
	if (HL > 0xFF) {
		F |= 0x10;
	}
	else {
		F &= 0xEF;
	}
	H = (HL >> 8) & 255;
	L = HL & 255;
	mTemp = 3; tTemp = 12;
}

void CPU::ADDHLSP() {
	int HL = (H << 8) | L;
	HL += SP;
	if (HL > 0xFF) {
		F |= 0x10;
	}
	else {
		F &= 0xEF;
	}
	H = (HL >> 8) & 255;
	L = HL & 255;
	mTemp = 3; tTemp = 12;

}

void CPU::ADDSPD() {
	uint8_t i = cpuBus->Read(PC);
	if (i >= 0x80) {
		i = -((~i + 1) & 0xFF);
	}
	PC++;
	SP += i;
	mTemp = 4;
	tTemp = 16;
}

void CPU::ADCAA() {
	A += A;
	A += (F & 0x10) ? 1 : 0;
	FZ(A);
	if (A > 0xFF) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::ADCAB() {
	A += B;
	A += (F & 0x10) ? 1 : 0;
	FZ(A);
	if (A > 0xFF) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::ADCAC() {
	A += C;
	A += (F & 0x10) ? 1 : 0;
	FZ(A);
	if (A > 0xFF) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::ADCAD() {
	A += D;
	A += (F & 0x10) ? 1 : 0;
	FZ(A);
	if (A > 0xFF) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::ADCAE() {
	A += E;
	A += (F & 0x10) ? 1 : 0;
	FZ(A);
	if (A > 0xFF) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::ADCAH() {
	A += H;
	A += (F & 0x10) ? 1 : 0;
	FZ(A);
	if (A > 0xFF) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::ADCAL() {
	A += L;
	A += (F & 0x10) ? 1 : 0;
	FZ(A);
	if (A > 0xFF) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::ADCAHL() {
	A += cpuBus->Read((H << 8) | L);
	A += (F & 0x10) ? 1 : 0;
	FZ(A);
	if (A > 0xFF) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 2; tTemp = 8;
}

void CPU::ADCA16() {
	A += cpuBus->Read(PC);
	PC++;
	A += (F & 0x10) ? 1 : 0;
	FZ(A);
	if (A > 0xFF) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 2; tTemp = 8;
}

void CPU::SUBAA() {
	A -= A;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::SUBAB() {
	A -= B;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::SUBAC() {
	A -= C;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::SUBAD() {
	A -= D;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::SUBAE() {
	A -= E;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::SUBAH() {
	A -= H;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::SUBAL() {
	A -= L;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::SUBAHL() {
	A -= cpuBus->Read((H << 8) | L);
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 2; tTemp = 8;
}

void CPU::SUBA8() {
	A -= cpuBus->Read(PC);
	FZ(A, true);
	PC++;
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 2; tTemp = 8;
}

void CPU::SBCAA() {
	A -= A;
	A -= (F & 0x10) ? 1 : 0;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::SBCAB() {
	A -= B;
	A -= (F & 0x10) ? 1 : 0;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::SBCAC() {
	A -= C;
	A -= (F & 0x10) ? 1 : 0;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::SBCAD() {
	A -= D;
	A -= (F & 0x10) ? 1 : 0;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::SBCAE() {
	A -= E;
	A -= (F & 0x10) ? 1 : 0;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::SBCAH() {
	A -= H;
	A -= (F & 0x10) ? 1 : 0;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::SBCAL() {
	A -= L;
	A -= (F & 0x10) ? 1 : 0;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 1; tTemp = 4;
}

void CPU::SBCAHL() {
	A -= cpuBus->Read(H << 8 | L);
	A -= (F & 0x10) ? 1 : 0;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 2; tTemp = 8;
}

void CPU::SBCA8() {
	A -= cpuBus->Read(PC);
	PC++;
	A -= (F & 0x10) ? 1 : 0;
	FZ(A, true);
	if (A < 0) {
		F |= 0x10;
	}
	A &= 0xFF;
	mTemp = 2; tTemp = 8;
}

void CPU::CPAA() {
	uint8_t i = A;
	i -= A;
	F |= 0x40;
	if (!(i & 0xFF)) { F |= 0x80; }
	if (i < 0) { F |= 0x10; }
	mTemp = 1; tTemp = 4;
}

void CPU::CPAB() {
	uint8_t i = A;
	i -= B;
	F |= 0x40;
	if (!(i & 0xFF)) { F |= 0x80; }
	if (i < 0) { F |= 0x10; }
	mTemp = 1; tTemp = 4;
}

void CPU::CPAC() {
	int i = A;
	i -= C;
	F |= 0x40;
	if (!(i & 0xFF)) { F |= 0x80; }
	if (i < 0) { F |= 0x10; }
	mTemp = 1; tTemp = 4;
}

void CPU::CPAD() {
	int i = A;
	i -= D;
	F |= 0x40;
	if (!(i & 0xFF)) { F |= 0x80; }
	if (i < 0) { F |= 0x10; }
	mTemp = 1; tTemp = 4;
}

void CPU::CPAE() {
	int i = A;
	i -= E;
	F |= 0x40;
	if (!(i & 0xFF)) { F |= 0x80; }
	if (i < 0) { F |= 0x10; }
	mTemp = 1; tTemp = 4;
}

void CPU::CPAH() {
	int i = A;
	i -= H;
	F |= 0x40;
	if (!(i & 0xFF)) { F |= 0x80; }
	if (i < 0) { F |= 0x10; }
	mTemp = 1; tTemp = 4;
}

void CPU::CPAL() {
	int i = A;
	i -= L;
	F |= 0x40;
	if (!(i & 0xFF)) { F |= 0x80; }
	if (i < 0) { F |= 0x10; }
	mTemp = 1; tTemp = 4;
}

void CPU::PUSHBC() {
	SP--;
	cpuBus->Write(SP, B);
	SP--;
	cpuBus->Write(SP, C);
	mTemp = 3; tTemp = 12;
}

void CPU::PUSHAF() {
	SP--;
	cpuBus->Write(SP, A);
	SP--;
	cpuBus->Write(SP, F);
	mTemp = 3; tTemp = 12;
}

void CPU::PUSHDE() {
	SP--;
	cpuBus->Write(SP, D);
	SP--;
	cpuBus->Write(SP, E);
	mTemp = 3; tTemp = 12;
}

void CPU::PUSHHL() {
	SP--;
	cpuBus->Write(SP, H);
	SP--;
	cpuBus->Write(SP, L);
	mTemp = 3; tTemp = 12;
}

void CPU::POPBC() {
	C = cpuBus->Read(SP);
	SP++;
	B = cpuBus->Read(SP);
	SP++;
	mTemp = 3; tTemp = 12;
}

void CPU::POPAF() {
	F = cpuBus->Read(SP);
	F &= 0xF0;
	SP++;
	A = cpuBus->Read(SP);
	SP++;
	mTemp = 3; tTemp = 12;
}

void CPU::POPDE() {
	E = cpuBus->Read(SP);
	SP++;
	D = cpuBus->Read(SP);
	SP++;
	mTemp = 3; tTemp = 12;
}

void CPU::POPHL() {
	L = cpuBus->Read(SP);
	SP++;
	H = cpuBus->Read(SP);
	SP++;
	mTemp = 3; tTemp = 12;
}

void CPU::LDABC() {
	int addr = C | (B << 8);
	A = cpuBus->Read(addr);
	mTemp = 2; tTemp = 8;
}

void CPU::LDADE() {
	int addr = E | (D << 8);
	A = cpuBus->Read(addr);
	mTemp = 2; tTemp = 8;
}

void CPU::LDAA() {
	A = A;
	mTemp = 1; tTemp = 4;
}

void CPU::LDAB() {
	A = B;
	mTemp = 1; tTemp = 4;
}

void CPU::LDAC() {
	A = C;
	mTemp = 1; tTemp = 4;
}

void CPU::LDAD() {
	A = D;
	mTemp = 1; tTemp = 4;
}

void CPU::LDAE() {
	A = E;
	mTemp = 1; tTemp = 4;
}

void CPU::LDAH() {
	A = H;
	mTemp = 1; tTemp = 4;
}

void CPU::LDAL() {
	A = L;
	mTemp = 1; tTemp = 4;
}

void CPU::LDBA() {
	B = A;
	mTemp = 1; tTemp = 4;
}

void CPU::LDBB() {
	B = B;
	mTemp = 1; tTemp = 4;
}

void CPU::LDBC() {
	B = C;
	mTemp = 1; tTemp = 4;
}

void CPU::LDBD() {
	B = D;
	mTemp = 1; tTemp = 4;
}

void CPU::LDBE() {
	B = E;
	mTemp = 1; tTemp = 4;
}

void CPU::LDBH() {
	B = H;
	mTemp = 1; tTemp = 4;
}

void CPU::LDBL() {
	B = L;
	mTemp = 1; tTemp = 4;
}

void CPU::LDCA() {
	C = A;
	mTemp = 1; tTemp = 4;
}

void CPU::LDCB() {
	C = B;
	mTemp = 1; tTemp = 4;
}

void CPU::LDCC() {
	C = C;
	mTemp = 1; tTemp = 4;
}

void CPU::LDCD() {
	C = D;
	mTemp = 1; tTemp = 4;
}

void CPU::LDCE() {
	C = E;
	mTemp = 1; tTemp = 4;
}

void CPU::LDCH() {
	C = H;
	mTemp = 1; tTemp = 4;
}

void CPU::LDCL() {
	C = L;
	mTemp = 1; tTemp = 4;
}

void CPU::LDDA() {
	D = A;
	mTemp = 1; tTemp = 4;
}

void CPU::LDDB() {
	D = B;
	mTemp = 1; tTemp = 4;
}

void CPU::LDDC() {
	D = C;
	mTemp = 1; tTemp = 4;
}

void CPU::LDDD() {
	D = D;
	mTemp = 1; tTemp = 4;
}

void CPU::LDDE() {
	D = E;
	mTemp = 1; tTemp = 4;
}

void CPU::LDDH() {
	D = H;
	mTemp = 1; tTemp = 4;
}

void CPU::LDDL() {
	D = L;
	mTemp = 1; tTemp = 4;
}

void CPU::LDEA() {
	E = A;
	mTemp = 1; tTemp = 4;
}

void CPU::LDEB() {
	E = B;
	mTemp = 1; tTemp = 4;
}

void CPU::LDEC() {
	E = C;
	mTemp = 1; tTemp = 4;
}

void CPU::LDED() {
	E = D;
	mTemp = 1; tTemp = 4;
}

void CPU::LDEE() {
	E = E;
	mTemp = 1; tTemp = 4;
}

void CPU::LDEH() {
	E = H;
	mTemp = 1; tTemp = 4;
}

void CPU::LDEL() {
	E = L;
	mTemp = 1; tTemp = 4;
}

void CPU::LDHA() {
	H = A;
	mTemp = 1; tTemp = 4;
}

void CPU::LDHB() {
	H = B;
	mTemp = 1; tTemp = 4;
}

void CPU::LDHC() {
	H = C;
	mTemp = 1; tTemp = 4;
}

void CPU::LDHD() {
	H = D;
	mTemp = 1; tTemp = 4;
}

void CPU::LDHE() {
	H = E;
	mTemp = 1; tTemp = 4;
}

void CPU::LDHH() {
	H = H;
	mTemp = 1; tTemp = 4;
}

void CPU::LDHL() {
	H = L;
	mTemp = 1; tTemp = 4;
}

void CPU::LDLA() {
	L = A;
	mTemp = 1; tTemp = 4;
}

void CPU::LDLB() {
	L = B;
	mTemp = 1; tTemp = 4;
}

void CPU::LDLC() {
	L = C;
	mTemp = 1; tTemp = 4;
}

void CPU::LDLD() {
	L = D;
	mTemp = 1; tTemp = 4;
}

void CPU::LDLE() {
	L = E;
	mTemp = 1; tTemp = 4;
}

void CPU::LDLH() {
	L = H;
	mTemp = 1; tTemp = 4;
}

void CPU::LDLL() {
	L = L;
	mTemp = 1; tTemp = 4;
}

void CPU::LDAHL() {
	A = cpuBus->Read((H << 8) | L);
	mTemp = 2; tTemp = 8;
}

void CPU::LDBHL() {
	B = cpuBus->Read((H << 8) | L);
	mTemp = 2; tTemp = 8;
}

void CPU::LDCHL() {
	C = cpuBus->Read((H << 8) | L);
	mTemp = 2; tTemp = 8;
}

void CPU::LDDHL() {
	D = cpuBus->Read((H << 8) | L);
	mTemp = 2; tTemp = 8;
}

void CPU::LDEHL() {
	E = cpuBus->Read((H << 8) | L);
	mTemp = 2; tTemp = 8;
}

void CPU::LDHHL() {
	H = cpuBus->Read((H << 8) | L);
	mTemp = 2; tTemp = 8;
}

void CPU::LDLHL() {
	L = cpuBus->Read((H << 8) | L);
	mTemp = 2; tTemp = 8;
}

void CPU::LDHLA() {
	cpuBus->Write((H << 8) | L, A);
	mTemp = 2; tTemp = 8;
}

void CPU::LDHLB() {
	cpuBus->Write((H << 8) | L, B);
	mTemp = 2; tTemp = 8;
}

void CPU::LDHLC() {
	cpuBus->Write((H << 8) | L, C);
	mTemp = 2; tTemp = 8;
}

void CPU::LDHLD() {
	cpuBus->Write((H << 8) | L, D);
	mTemp = 2; tTemp = 8;
}

void CPU::LDHLE() {
	cpuBus->Write((H << 8) | L, E);
	mTemp = 2; tTemp = 8;
}

void CPU::LDHLH() {
	cpuBus->Write((H << 8) | L, H);
	mTemp = 2; tTemp = 8;
}

void CPU::LDHLL() {
	cpuBus->Write((H << 8) | L, L);
	mTemp = 2; tTemp = 8;
}

void CPU::LDA8() {
	A = cpuBus->Read(PC);
	PC++;
	mTemp = 2; tTemp = 8;
}

void CPU::LDB8() {
	B = cpuBus->Read(PC);
	PC++;
	mTemp = 2; tTemp = 8;
}

void CPU::LDC8() {
	C = cpuBus->Read(PC);
	PC++;
	mTemp = 2; tTemp = 8;
}

void CPU::LDD8() {
	D = cpuBus->Read(PC);
	PC++;
	mTemp = 2; tTemp = 8;
}

void CPU::LDE8() {
	E = cpuBus->Read(PC);
	PC++;
	mTemp = 2; tTemp = 8;
}

void CPU::LDH8() {
	H = cpuBus->Read(PC);
	PC++;
	mTemp = 2; tTemp = 8;
}

void CPU::LDL8() {
	L = cpuBus->Read(PC);
	PC++;
	mTemp = 2; tTemp = 8;
}

void CPU::LDHL8() {
	cpuBus->Write((H << 8) | L, cpuBus->Read(PC));
	PC++;
	mTemp = 3; tTemp = 12;
}

void CPU::LDBCA() {
	cpuBus->Write((B << 8) | C, A);
	mTemp = 2; tTemp = 8;
}

void CPU::LDDEA() {
	cpuBus->Write((D << 8) | E, A);
	mTemp = 2; tTemp = 8;
}

void CPU::LD16A() {
	cpuBus->Write(cpuBus->ReadL(PC), A);
	PC += 2;
	mTemp = 4; tTemp = 16;
}

void CPU::LDA16() {
	A = cpuBus->Read(cpuBus->ReadL(PC));
	PC += 2;
	mTemp = 4; tTemp = 16;
}

void CPU::LDBC16() {
	C = cpuBus->Read(PC);
	B = cpuBus->Read(PC + 1);
	PC += 2;
	mTemp = 3; tTemp = 12;
}

void CPU::LDDE16() {
	E = cpuBus->Read(PC);
	D = cpuBus->Read(PC + 1);
	PC += 2;
	mTemp = 3; tTemp = 12;
}

void CPU::LDHL16() {
	L = cpuBus->Read(PC);
	H = cpuBus->Read(PC + 1);
	PC += 2;
	mTemp = 3; tTemp = 12;
}

void CPU::LD16SP() {
	SP = cpuBus->ReadL(PC);
	PC += 2;
	mTemp = 3; tTemp = 12;
}

void CPU::INCA() {
	A++;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::INCB() {
	B++;
	B &= 0xFF;
	FZ(B);
	mTemp = 1; tTemp = 4;
}

void CPU::INCC() {
	C++;
	C &= 0xFF;
	FZ(C);
	mTemp = 1; tTemp = 4;
}

void CPU::INCD() {
	D++;
	D &= 0xFF;
	FZ(D);
	mTemp = 1; tTemp = 4;
}

void CPU::INCE() {
	E++;
	E &= 0xFF;
	FZ(E);
	mTemp = 1; tTemp = 4;
}

void CPU::INCH() {
	H++;
	H &= 0xFF;
	FZ(H);
	mTemp = 1; tTemp = 4;
}

void CPU::INCL() {
	L++;
	L &= 0xFF;
	FZ(L);
	mTemp = 1; tTemp = 4;
}

void CPU::INCHLR() {
	int i = cpuBus->Read((H << 8) | L) + 1;
	i &= 0xFF;
	cpuBus->Write((H << 8) | L, i);
	FZ(i);
	mTemp = 3; tTemp = 12;
}

void CPU::DECA() {
	A--;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::DECB() {
	B--;
	B &= 0xFF;
	FZ(B);
	mTemp = 1; tTemp = 4;
}

void CPU::DECC() {
	C--;
	C &= 0xFF;
	FZ(C);
	mTemp = 1; tTemp = 4;
}

void CPU::DECD() {
	D--;
	D &= 0xFF;
	FZ(D);
	mTemp = 1; tTemp = 4;
}

void CPU::DECE() {
	E--;
	E &= 0xFF;
	FZ(E);
	mTemp = 1; tTemp = 4;
}

void CPU::DECH() {
	H--;
	H &= 0xFF;
	FZ(H);
	mTemp = 1; tTemp = 4;
}

void CPU::DECL() {
	L--;
	L &= 0xFF;
	FZ(L);
	mTemp = 1; tTemp = 4;
}

void CPU::DECHLR() {
	int i = cpuBus->Read((H << 8) | L) - 1;
	i &= 0xFF;
	cpuBus->Write((H << 8) | L, i);
	FZ(i);
	mTemp = 3; tTemp = 12;
}

void CPU::INCBC() {
	C = (C + 1) & 0xFF;
	if (!C) {
		B = (B + 1) & 0xFF;
	}
	mTemp = 1;
	tTemp = 4;
}

void CPU::INCDE() {
	E = (E + 1) & 0xFF;
	if (!E) {
		D = (D + 1) & 0xFF;
	}
	mTemp = 1;
	tTemp = 4;
}

void CPU::INCHL() {
	L = (L + 1) & 0xFF;
	if (!L) {
		H = (H + 1) & 0xFF;
	}
	mTemp = 1;
	tTemp = 4;
}

void CPU::INCSP() {
	SP = (SP + 1) & 0xFFFF;
	mTemp = 1;
	tTemp = 4;
}

void CPU::DECBC() {
	C = (C - 1) & 0xFF;
	if (C == 0xFF) {
		B = (B - 1) & 0xFF;
	}
	mTemp = 1;
	tTemp = 4;
}

void CPU::DECDE() {
	E = (E - 1) & 0xFF;
	if (E == 0xFF) {
		D = (D - 1) & 0xFF;
	}
	mTemp = 1;
	tTemp = 4;
}

void CPU::DECHL() {
	L = (L - 1) & 0xFF;
	if (L == 0xFF) {
		H = (H - 1) & 0xFF;
	}
	mTemp = 1;
	tTemp = 4;
}

void CPU::DECSP() {
	SP = (SP - 1) & 0xFFFF;
	mTemp = 1;
	tTemp = 4;
}

void CPU::JP16() {
	PC = cpuBus->ReadL(PC);
	mTemp = 3; tTemp = 12;
}

void CPU::JPHL() {
	PC = (H << 8) | L;
	mTemp = 1; tTemp = 4;
}

void CPU::JPNZ16() {
	mTemp = 3; tTemp = 12;
	if ((F & 0x80) == 0x00) {
		PC = cpuBus->ReadL(PC);
		mTemp++; tTemp += 4;
	}
	else
		PC += 2;
}

void CPU::JPZ16() {
	mTemp = 3; tTemp = 12;
	if ((F & 0x80) == 0x80) {
		PC = cpuBus->ReadL(PC);
		mTemp++; tTemp += 4;
	}
	else
		PC += 2;
}

void CPU::JPNC16() {
	mTemp = 3; tTemp = 12;
	if ((F & 0x10) == 0x00) {
		PC = cpuBus->ReadL(PC);
		mTemp++; tTemp += 4;
	}
	else
		PC += 2;
}

void CPU::JPC16() {
	mTemp = 3; tTemp = 12;
	if ((F & 0x10) == 0x10) {
		PC = cpuBus->ReadL(PC);
		mTemp++; tTemp += 4;
	}
	else
		PC += 2;
}

void CPU::JR8() {
	int i = cpuBus->Read(PC);
	if (i >= 0x80)
		i = -((~i + 1) & 255);
	PC++;
	mTemp = 2; tTemp = 8;
	PC += i;
	mTemp++; tTemp += 4;
}

void CPU::JRNZ8() {
	int i = cpuBus->Read(PC);
	if (i > 127)
		i = -((~i + 1) & 255);
	PC++;
	mTemp = 2; tTemp = 8;
	if ((F & 0x80) == 0x00) { 
		PC += i;
		mTemp++; tTemp += 4;
	}
}

void CPU::JRZ8() {
	int i = cpuBus->Read(PC);
	if (i >= 0x80)
		i = -((~i + 1) & 255);
	PC++;
	mTemp = 2; tTemp = 8;
	if ((F & 0x80) == 0x80) {
		PC += i;
		mTemp += 1; tTemp += 4;
	}
}

void CPU::JRNC8() {
	int i = cpuBus->Read(PC);
	if (i >= 0x80)
		i = -((~i + 1) & 255);
	PC++;
	mTemp = 2; tTemp = 8;
	if ((F & 0x10) == 0x00) {
		PC += i;
		mTemp++; tTemp += 4;
	}
}

void CPU::JRC8() {
	int i = cpuBus->Read(PC);
	if (i >= 0x80)
		i = -((~i + 1) & 255);
	PC++;
	mTemp = 2; tTemp = 8;
	if ((F & 0x10) == 0x10) {
		PC += i;
		mTemp++; tTemp += 4;
	}
}

void CPU::ANDA() {
	A &= A;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::ANDB() {
	A &= B;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::ANDC() {
	A &= C;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::ANDD() {
	A &= D;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::ANDE() {
	A &= E;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::ANDH() {
	A &= H;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::ANDL() {
	A &= L;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::ANDHL() {
	A &= cpuBus->Read((H << 8) | L);
	A &= 0xFF;
	FZ(A);
	mTemp = 2; tTemp = 8;
}

void CPU::AND8() {
	A &= cpuBus->Read(PC);
	A &= 0xFF;
	PC++;
	F = (A ? 0 : 0x80);
	mTemp = 2; tTemp = 8;
}

void CPU::ORA() {
	A |= A;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::ORB() {
	A |= B;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::ORC() {
	A |= C;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::ORD() {
	A |= D;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::ORE() {
	A |= E;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::ORH() {
	A |= H;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::ORL() {
	A |= L;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::ORHL() {
	A |= cpuBus->Read((H << 8) | L);
	A &= 0xFF;
	F = A ? 0 : 0x80;
	mTemp = 2; tTemp = 8;
}

void CPU::OR8() {
	A |= cpuBus->Read(PC);
	A &= 0xFF;
	PC++;
	FZ(A);
	mTemp = 2; tTemp = 8;
}

void CPU::XORA() {
	A ^= A;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::XORB() {
	A ^= B;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::XORC() {
	A ^= C;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::XORD() {
	A ^= D;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::XORE() {
	A ^= E;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::XORH() {
	A ^= H;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::XORL() {
	A ^= L;
	A &= 0xFF;
	FZ(A);
	mTemp = 1; tTemp = 4;
}

void CPU::XORHL() {
	A ^= cpuBus->Read((H << 8) | L);
	A &= 0xFF;
	FZ(A);
	mTemp = 2; tTemp = 8;
}

void CPU::XOR8() {
	A ^= cpuBus->Read(PC);
	A &= 0xFF;
	PC++;
	FZ(A);
	mTemp = 2; tTemp = 8;
}

void CPU::NOP() {
	mTemp = 1; tTemp = 4;
}

void CPU::STOP() {
	int i = cpuBus->Read(PC);
	if (i >= 0x80) {
		i = -((~i + 1) & 0xFF);
	}
	PC++;
	mTemp = 2;
	tTemp = 8;
	B--;
	if (B != 0) {
		PC += i;
		mTemp++; tTemp += 4;
	}
}

void CPU::RET() {
	PC = cpuBus->ReadL(SP);
	SP += 2;
	mTemp = 3; tTemp = 12;
}

void CPU::RETI() {
	IME = 1;
	RRS();
	PC = cpuBus->ReadL(SP);
	SP += 2;
	mTemp = 3; tTemp = 12;
}

void CPU::RETNZ() {
	mTemp = 1; tTemp = 4;
	if ((F & 0x80) == 0x00) {
		PC = cpuBus->ReadL(SP);
		SP += 2;
		mTemp += 2; tTemp += 8;
	}
}

void CPU::RETZ() {
	mTemp = 1; tTemp = 4;
	if ((F & 0x80) == 0x80) {
		PC = cpuBus->ReadL(SP);
		SP += 2;
		mTemp += 2; tTemp += 8;
	}
}

void CPU::RETNC() {
	mTemp = 1; tTemp = 4;
	if ((F & 0x10) == 0x00) {
		PC = cpuBus->ReadL(SP);
		SP += 2;
		mTemp += 2; tTemp += 8;
	}
}

void CPU::RETC() {
	mTemp = 1; tTemp = 4;
	if ((F & 0x10) == 0x10) {
		PC = cpuBus->ReadL(SP);
		SP += 2;
		mTemp += 2; tTemp += 8;
	}
}

void CPU::RST0() {
	RSV();
	SP -= 2;
	cpuBus->WriteL(SP, PC);
	PC = 0x00;
	mTemp = 3; tTemp = 12;
}

void CPU::RST8() {
	RSV();
	SP -= 2;
	cpuBus->WriteL(SP, PC);
	PC = 0x08;
	mTemp = 3; tTemp = 12;
}

void CPU::RST10() {
	RSV();
	SP -= 2;
	cpuBus->WriteL(SP, PC);
	PC = 0x10;
	mTemp = 3; tTemp = 12;
}

void CPU::RST18() {
	RSV();
	SP -= 2;
	cpuBus->WriteL(SP, PC);
	PC = 0x18;
	mTemp = 3; tTemp = 12;
}

void CPU::RST20() {
	RSV();
	SP -= 2;
	cpuBus->WriteL(SP, PC);
	PC = 0x20;
	mTemp = 3; tTemp = 12;
}

void CPU::RST28() {
	RSV();
	SP -= 2;
	cpuBus->WriteL(SP, PC);
	PC = 0x28;
	mTemp = 3; tTemp = 12;
}

void CPU::RST30() {
	RSV();
	SP -= 2;
	cpuBus->WriteL(SP, PC);
	PC = 0x30;
	mTemp = 3; tTemp = 12;
}

void CPU::RST38() {
	RSV();
	SP -= 2;
	cpuBus->WriteL(SP, PC);
	PC = 0x38;
	mTemp = 3; tTemp = 12;
}

void CPU::RST40() {
	RSV();
	SP -= 2;
	cpuBus->WriteL(SP, PC);
	PC = 0x40;
	mTemp = 3; tTemp = 12;
}

void CPU::RST48() {
	RSV();
	SP -= 2;
	cpuBus->WriteL(SP, PC);
	PC = 0x48;
	mTemp = 3; tTemp = 12;
}

void CPU::RST50() {
	RSV();
	SP -= 2;
	cpuBus->WriteL(SP, PC);
	PC = 0x50;
	mTemp = 3; tTemp = 12;
}

void CPU::RST58() {
	RSV();
	SP -= 2;
	cpuBus->WriteL(SP, PC);
	PC = 0x58;
	mTemp = 3; tTemp = 12;
}

void CPU::RST60() {
	RSV();
	SP -= 2;
	cpuBus->WriteL(SP, PC);
	PC = 0x60;
	mTemp = 3; tTemp = 12;
}

void CPU::DI() {
	IME = 0; mTemp = 1; tTemp = 4;
}

void CPU::EI() {
	IME = 1; mTemp = 1; tTemp = 4;
}

void CPU::RLA() {
	int i = (F & 0x10) ? 1 : 0;
	int o = (A & 0x80) ? 0x10 : 0;
	A = (A << 1) + i;
	A &= 0xFF;
	F = (F & 0xEF) + o;
	mTemp = 1; tTemp = 4;
}

void CPU::RLCA() {
	int i = (A & 0x80) ? 1 : 0;
	int o = (A & 0x80) ? 0x10 : 0;
	A = (A << 1) + i;
	A &= 0xFF;
	F = (F & 0xEF) + o;
	mTemp = 1; tTemp = 4;
}

void CPU::RRA() {
	int i = (F & 0x10) ? 0x80 : 0;
	int o = (A & 0x1) ? 0x10 : 0;
	A = (A >> 1) + i;
	A &= 0xFF;
	F = (F & 0xEF) + o;
	mTemp = 1; tTemp = 4;
}

void CPU::RRCA() {
	int i = (A & 0x1) ? 0x80 : 0;
	int o = (A & 0x1) ? 0x10 : 0;
	A = (A >> 1) + i;
	A &= 0xFF;
	F = (F & 0xEF) + o;
	mTemp = 1; tTemp = 4;
}

void CPU::CALL16() {
	SP -= 2;
	cpuBus->WriteL(SP, PC + 2);
	PC = cpuBus->ReadL(PC);
	mTemp = 5; tTemp = 20;
}

void CPU::CALLNZ16() {
	mTemp = 3; tTemp = 12;
	if ((F & 0x80) == 0x00) {
		SP -= 2;
		cpuBus->WriteL(SP, PC + 2);
		PC = cpuBus->ReadL(PC);
		mTemp += 2; tTemp += 8;
	}
	else {
		PC += 2;
	}
}

void CPU::CALLZ16() {
	mTemp = 3; tTemp = 12;
	if ((F & 0x80) == 0x80) {
		SP -= 2;
		cpuBus->WriteL(SP, PC + 2);
		PC = cpuBus->ReadL(PC);
		mTemp += 2; tTemp += 8;
	}
	else {
		PC += 2;
	}
}

void CPU::CALLNC16() {
	mTemp = 3; tTemp = 12;
	if ((F & 0x10) == 0x00) {
		SP -= 2;
		cpuBus->WriteL(SP, PC + 2);
		PC = cpuBus->ReadL(PC);
		mTemp += 2; tTemp += 8;
	}
	else {
		PC += 2;
	}
}

void CPU::CALLC16() {
	mTemp = 3; tTemp = 12;
	if ((F & 0x10) == 0x10) {
		SP -= 2;
		cpuBus->WriteL(SP, PC + 2);
		PC = cpuBus->ReadL(PC);
		mTemp += 2; tTemp += 8;
	}
	else {
		PC += 2;
	}
}

void CPU::LDSP16() {
	SP = cpuBus->ReadL(PC);
	PC += 2;
	mTemp = 3; tTemp = 12;
}

void CPU::LDDHLA() {
	cpuBus->Write((H << 8) | L, A);
	L = (L - 1) & 0xFF;
	if (L == 0xFF) {
		H = (H - 1) & 0xFF;
	}
	mTemp = 2; tTemp = 8;
}

void CPU::LDDAHL() {
	A = cpuBus->Read((H << 8) | L);
	L = (L - 1) & 0xFF;
	if (L == 0xFF) {
		H = (H - 1) & 0xFF;
	}
	mTemp = 2; tTemp = 8;
}

void CPU::LDIHLA() {
	cpuBus->Write((H << 8) | L, A);
	L = (L + 1) & 0xFF;
	if (!L) {
		H = (H + 1) & 0xFF;
	}
	mTemp = 2; tTemp = 8;
}

void CPU::LDIAHL() {
	A = cpuBus->Read((H << 8) | L);
	L = (L + 1) & 0xFF;
	if (!L) {
		H = (H + 1) & 0xFF;
	}
	mTemp = 2; tTemp = 8;
}

void CPU::CPL() {
	A = (~A) & 0xFF;
	FZ(A, true);
	mTemp = 1; tTemp = 4;
}

void CPU::SCF() {
	F |= 0x10;
	mTemp = 1; tTemp = 4;
}

void CPU::CCF() {
	int i = F & 0x10 ? 0 : 0x10;
	F = (F & 0xEF) + i;
	mTemp = 1; tTemp = 4;
}

void CPU::LDHLSPD() {
	int i = cpuBus->Read(PC);
	if (i >= 0x80) {
		i = -((~i + 1) & 0xFF);
	}
	PC++;
	i += SP;
	H = (i >> 8) & 0xFF;
	L = i & 0xFF;
	mTemp = 3; tTemp = 12;
}

void CPU::LDHA8() {
	int k = 0xFF00 + cpuBus->Read(PC);
	A = cpuBus->Read(0xFF00 + cpuBus->Read(PC));
	PC++;
	mTemp = 3; tTemp = 12;
}

void CPU::LDH8A() {
	cpuBus->Write(0xFF00 + cpuBus->Read(PC), A);
	PC++;
	mTemp = 3; tTemp = 12;
}

void CPU::LDHCA() {
	cpuBus->Write(0xFF00 + C, A);
	mTemp = 2; tTemp = 8;
}

void CPU::EXT() {
	int i = cpuBus->Read(PC);
	auto s = cbMap[i].name;
	PC++;
	PC &= 0xFFFF;
	if (i < 0xFF) {
		(this->*cbMap[i].op)();
	}
}

void CPU::CPAHL() {
	int i = A;
	i -= cpuBus->Read(((H << 8) | L));
	FZ(i, true);
	if (i < 0) {
		F |= 0x10;
	}
	i &= 0xFF;
	mTemp = 2; tTemp = 8;
}

void CPU::CP8() {
	int i = A;
	int m = cpuBus->Read(PC);
	i -= m;
	PC++;
	F = (i < 0) ? 0x50 : 0x40;
	i &= 0xFF;
	if (!i) {
		F |= 0x80;
	}
	if ((A ^ i ^ m) & 0x10) F |= 0x20;
	mTemp = 2; tTemp = 8;
}

void CPU::HALT() {
	halt = true;
	mTemp = 1; tTemp = 4;
}

void CPU::XXX() {
	stop = true;
	//std::cout << "Invalid instruction at 0x" + std::to_string(PC - 1) << std::endl;
	//std::cout << instructions[cpuBus->Read(PC - 1)].name << cpuBus->Read(PC - 1) << std::endl;
}



#pragma endregion

#pragma region TwoByteInstructions

void CPU::RLCB() {
	int i = B & 0x80 ? 1 : 0;
	int o = B & 0x80 ? 0x10 : 0;
	B = (B << 1) + i;
	B &= 0xFF;
	F = B ? 0 : 0x80;
	F = (F & 0xEF) + o;
	mTemp = 2;
}

void CPU::RLCC() {
	int i = C & 0x80 ? 1 : 0;
	int o = C & 0x80 ? 0x10 : 0;
	C = (C << 1) + i;
	C &= 0xFF;
	F = C ? 0 : 0x80;
	F = (F & 0xEF) + o;
	mTemp = 2;
}

void CPU::RLCD() {
	int i = D & 0x80 ? 1 : 0;
	int o = D & 0x80 ? 0x10 : 0;
	D = (D << 1) + i;
	D &= 0xFF;
	F = D ? 0 : 0x80;
	F = (F & 0xEF) + o;
	mTemp = 2;
}

void CPU::RLCE() {
	int i = E & 0x80 ? 1 : 0;
	int o = E & 0x80 ? 0x10 : 0;
	E = (E << 1) + i;
	E &= 0xFF;
	F = E ? 0 : 0x80;
	F = (F & 0xEF) + o;
	mTemp = 2;
}

void CPU::RLCH() {
	int i = H & 0x80 ? 1 : 0;
	int o = H & 0x80 ? 0x10 : 0;
	H = (H << 1) + i;
	H &= 0xFF;
	F = H ? 0 : 0x80;
	F = (F & 0xEF) + o;
	mTemp = 2;
}

void CPU::RLCL() {
	int i = L & 0x80 ? 1 : 0;
	int o = L & 0x80 ? 0x10 : 0;
	L = (L << 1) + i;
	L &= 0xFF;
	F = L ? 0 : 0x80;
	F = (F & 0xEF) + o;
	mTemp = 2;
}

void CPU::RLCAr() {
	int i = A & 0x80 ? 1 : 0;
	int o = A & 0x80 ? 0x10 : 0;
	A = (A << 1) + i;
	A &= 0xFF;
	F = A ? 0 : 0x80;
	F = (F & 0xEF) + o;
	mTemp = 2;
}

void CPU::RLCHL() {
	int i = cpuBus->Read((H << 8) | L);
	int ci = (i & 0x80) ? 1 : 0;
	int co = (i & 0x80) ? 0x10 : 0;
	i = (i << 1) + ci;
	i &= 0xFF;
	F = (i) ? 0 : 0x80;
	cpuBus->Write((H << 8) | L, i);
	F = (F & 0xEF) + co;
	mTemp = 4;
}

void CPU::RRCB() {
	int ci = B & 1 ? 0x80 : 0;
	int co = B & 1 ? 0x10 : 0;
	B = (B >> 1) + ci;
	B &= 0xFF;
	FZ(B);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RRCC() {
	int ci = C & 1 ? 0x80 : 0;
	int co = C & 1 ? 0x10 : 0;
	C = (C >> 1) + ci;
	C &= 0xFF;
	FZ(C);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RRCD() {
	int ci = D & 1 ? 0x80 : 0;
	int co = D & 1 ? 0x10 : 0;
	D = (D >> 1) + ci;
	D &= 0xFF;
	FZ(D);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RRCE() {
	int ci = E & 1 ? 0x80 : 0;
	int co = E & 1 ? 0x10 : 0;
	E = (E >> 1) + ci;
	E &= 0xFF;
	FZ(E);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RRCH() {
	int ci = H & 1 ? 0x80 : 0;
	int co = H & 1 ? 0x10 : 0;
	H = (H >> 1) + ci;
	H &= 0xFF;
	FZ(H);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RRCL() {
	int ci = L & 1 ? 0x80 : 0;
	int co = L & 1 ? 0x10 : 0;
	L = (L >> 1) + ci;
	L &= 0xFF;
	FZ(L);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RRCAr() {
	int ci = A & 1 ? 0x80 : 0;
	int co = A & 1 ? 0x10 : 0;
	A = (A >> 1) + ci;
	A &= 0xFF;
	FZ(A);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RRCHL() {
	int i = cpuBus->Read((H << 8) | L);
	int ci = i & 1 ? 0x80 : 0;
	int co = i & 1 ? 0x10 : 0;
	i = (i >> 1) + ci;
	i &= 0xFF;
	cpuBus->Write((H << 8) | L, i);
	F = (i) ? 0 : 0x80;
	F = (F & 0xEF) + co;
	mTemp = 4; tTemp = 16;
}

void CPU::RLB() {
	int ci = F & 0x10 ? 1 : 0;
	int co = B & 0x80 ? 0x10 : 0;
	B = (B << 1) + ci;
	B &= 0xFF;
	FZ(B);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RLC() {
	int ci = F & 0x10 ? 1 : 0;
	int co = C & 0x80 ? 0x10 : 0;
	C = (C << 1) + ci;
	C &= 0xFF;
	FZ(C);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RLD() {
	int ci = F & 0x10 ? 1 : 0;
	int co = D & 0x80 ? 0x10 : 0;
	D = (D << 1) + ci;
	D &= 0xFF;
	FZ(D);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RLE() {
	int ci = F & 0x10 ? 1 : 0;
	int co = E & 0x80 ? 0x10 : 0;
	E = (E << 1) + ci;
	E &= 0xFF;
	FZ(E);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RLH() {
	int ci = F & 0x10 ? 1 : 0;
	int co = H & 0x80 ? 0x10 : 0;
	H = (H << 1) + ci;
	H &= 0xFF;
	FZ(H);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RLL() {
	int ci = F & 0x10 ? 1 : 0;
	int co = L & 0x80 ? 0x10 : 0;
	L = (L << 1) + ci;
	L &= 0xFF;
	FZ(L);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RLAr() {
	int ci = F & 0x10 ? 1 : 0;
	int co = A & 0x80 ? 0x10 : 0;
	A = (A << 1) + ci;
	A &= 0xFF;
	FZ(A);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RLHL() {
	int i = cpuBus->Read((H << 8) | L);
	int ci = F & 0x10 ? 1 : 0;
	int co = i & 0x80 ? 0x10 : 0;
	i = (i << 1) + ci;
	i &= 0xFF;
	FZ(i);
	cpuBus->Write((H << 8) | L, i);
	F = (F & 0xEF) + co;
	mTemp = 4; tTemp = 16;
}

void CPU::RRB() {
	int ci = F & 0x10 ? 0x80 : 0;
	int co = B & 1 ? 0x10 : 0;
	B = (B >> 1) + ci;
	B &= 0xFF;
	FZ(B);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RRC() {
	int ci = F & 0x10 ? 0x80 : 0;
	int co = C & 1 ? 0x10 : 0;
	C = (C >> 1) + ci;
	C &= 0xFF;
	FZ(C);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RRD() {
	int ci = F & 0x10 ? 0x80 : 0;
	int co = D & 1 ? 0x10 : 0;
	D = (D >> 1) + ci;
	D &= 0xFF;
	FZ(D);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RRE() {
	int ci = F & 0x10 ? 0x80 : 0;
	int co = E & 1 ? 0x10 : 0;
	E = (E >> 1) + ci;
	E &= 0xFF;
	FZ(E);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RRH() {
	int ci = F & 0x10 ? 0x80 : 0;
	int co = H & 1 ? 0x10 : 0;
	H = (H >> 1) + ci;
	H &= 0xFF;
	FZ(H);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RRL() {
	int ci = F & 0x10 ? 0x80 : 0;
	int co = L & 1 ? 0x10 : 0;
	L = (L >> 1) + ci;
	L &= 0xFF;
	FZ(L);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RRAr() {
	int ci = F & 0x10 ? 0x80 : 0;
	int co = A & 1 ? 0x10 : 0;
	A = (A >> 1) + ci;
	A &= 0xFF;
	FZ(A);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::RRHL() {
	int i = cpuBus->Read((H << 8) | L);
	int ci = F & 0x10 ? 1 : 0;
	int co = i & 0x80 ? 0x10 : 0;
	i = (i << 1) + ci;
	i &= 0xFF;
	FZ(i);
	cpuBus->Write((H << 8) | L, i);
	F = (F & 0xEF) + co;
	mTemp = 4; tTemp = 16;
}

void CPU::SLAB() {
	int co = B & 0x80 ? 0x10 : 0;
	B = (B << 1) & 0xFF;
	FZ(B);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SLAC() {
	int co = C & 0x80 ? 0x10 : 0;
	C = (C << 1) & 0xFF;
	FZ(C);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SLAD() {
	int co = D & 0x80 ? 0x10 : 0;
	D = (D << 1) & 0xFF;
	FZ(D);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SLAE() {
	int co = E & 0x80 ? 0x10 : 0;
	E = (E << 1) & 0xFF;
	FZ(E);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SLAH() {
	int co = H & 0x80 ? 0x10 : 0;
	H = (H << 1) & 0xFF;
	FZ(H);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SLAL() {
	int co = L & 0x80 ? 0x10 : 0;
	L = (L << 1) & 0xFF;
	FZ(L);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SLAA() {
	int co = A & 0x80 ? 0x10 : 0;
	A = (A << 1) & 0xFF;
	FZ(A);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SRAB() {
	int ci = B & 0x80;
	int co = B & 1 ? 0x10 : 0;
	B = ((B >> 1) + ci) & 0xFF;
	FZ(B);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SRAC() {
	int ci = C & 0x80;
	int co = C & 1 ? 0x10 : 0;
	C = ((C >> 1) + ci) & 0xFF;
	FZ(C);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SRAD() {
	int ci = D & 0x80;
	int co = D & 1 ? 0x10 : 0;
	D = ((D >> 1) + ci) & 0xFF;
	FZ(D);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SRAE() {
	int ci = E & 0x80;
	int co = E & 1 ? 0x10 : 0;
	E = ((E >> 1) + ci) & 0xFF;
	FZ(E);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SRAH() {
	int ci = H & 0x80;
	int co = H & 1 ? 0x10 : 0;
	H = ((H >> 1) + ci) & 0xFF;
	FZ(H);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SRAL() {
	int ci = L & 0x80;
	int co = L & 1 ? 0x10 : 0;
	L = ((L >> 1) + ci) & 0xFF;
	FZ(L);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SRAA() {
	int ci = A & 0x80;
	int co = A & 1 ? 0x10 : 0;
	A = ((A >> 1) + ci) & 0xFF;
	FZ(A);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SWAPB() {
	int tr = B;
	B = (cpuBus->Read((H << 8) | L));
	cpuBus->Write((H << 8) | L, tr);
	mTemp = 4; tTemp = 16;
}

void CPU::SWAPC() {
	int tr = C;
	C = (cpuBus->Read((H << 8) | L));
	cpuBus->Write((H << 8) | L, tr);
	mTemp = 4; tTemp = 16;
}

void CPU::SWAPD() {
	int tr = D;
	D = (cpuBus->Read((H << 8) | L));
	cpuBus->Write((H << 8) | L, tr);
	mTemp = 4; tTemp = 16;
}

void CPU::SWAPE() {
	int tr = E;
	E = (cpuBus->Read((H << 8) | L));
	cpuBus->Write((H << 8) | L, tr);
	mTemp = 4; tTemp = 16;
}

void CPU::SWAPH() {
	int tr = H;
	H = (cpuBus->Read((H << 8) | L));
	cpuBus->Write((H << 8) | L, tr);
	mTemp = 4; tTemp = 16;
}

void CPU::SWAPL() {
	int tr = L;
	L = (cpuBus->Read((H << 8) | L));
	cpuBus->Write((H << 8) | L, tr);
	mTemp = 4; tTemp = 16;
}

void CPU::SWAPA() {
	int tr = A;
	A = (cpuBus->Read((H << 8) | L));
	cpuBus->Write((H << 8) | L, tr);
	mTemp = 4; tTemp = 16;
}

void CPU::SRLB() {
	int co = B & 1 ? 0x10 : 0;
	B = (B >> 1) & 0xFF;
	FZ(B);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SRLC() {
	int co = C & 1 ? 0x10 : 0;
	C = (C >> 1) & 0xFF;
	FZ(C);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SRLD() {
	int co = D & 1 ? 0x10 : 0;
	D = (D >> 1) & 0xFF;
	FZ(D);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SRLE() {
	int co = E & 1 ? 0x10 : 0;
	E = (E >> 1) & 0xFF;
	FZ(E);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SRLH() {
	int co = H & 1 ? 0x10 : 0;
	H = (H >> 1) & 0xFF;
	FZ(H);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SRLL() {
	int co = L & 1 ? 0x10 : 0;
	L = (L >> 1) & 0xFF;
	FZ(L);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::SRLA() {
	int co = A & 1 ? 0x10 : 0;
	A = (A >> 1) & 0xFF;
	FZ(A);
	F = (F & 0xEF) + co;
	mTemp = 2; tTemp = 8;
}

void CPU::BIT0B() {
	FZ(B & 1);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT0C() {
	FZ(C & 1);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT0D() {
	FZ(D & 1);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT0E() {
	FZ(E & 1);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT0H() {
	FZ(H & 1);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT0L() {
	FZ(L & 1);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT0A() {
	FZ(A & 1);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT0M() {
	FZ(cpuBus->Read((H << 8) | L) & 1);
	mTemp = 3; tTemp = 12;
}

void CPU::BIT1B() {
	FZ(B & 2);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT1C() {
	FZ(C & 2);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT1D() {
	FZ(D & 2);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT1E() {
	FZ(E & 2);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT1H() {
	FZ(H & 2);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT1L() {
	FZ(L & 2);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT1A() {
	FZ(A & 2);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT1M() {
	FZ(cpuBus->Read((H << 8) | L) & 2);
	mTemp = 3; tTemp = 12;
}

void CPU::BIT2B() {
	FZ(B & 4);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT2C() {
	FZ(C & 4);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT2D() {
	FZ(D & 4);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT2E() {
	FZ(E & 4);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT2H() {
	FZ(H & 4);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT2L() {
	FZ(L & 4);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT2A() {
	FZ(A & 4);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT2M() {
	FZ(cpuBus->Read((H << 8) | L) & 4);
	mTemp = 3; tTemp = 12;
}

void CPU::BIT3B() {
	FZ(B & 8);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT3C() {
	FZ(C & 8);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT3D() {
	FZ(D & 8);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT3E() {
	FZ(E & 8);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT3H() {
	FZ(H & 8);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT3L() {
	FZ(L & 8);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT3A() {
	FZ(A & 8);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT3M() {
	FZ(cpuBus->Read((H << 8) | L) & 8);
	mTemp = 3; tTemp = 12;
}

void CPU::BIT4B() {
	FZ(B & 0x10);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT4C() {
	FZ(C & 0x10);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT4D() {
	FZ(D & 0x10);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT4E() {
	FZ(E & 0x10);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT4H() {
	FZ(H & 0x10);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT4L() {
	FZ(L & 0x10);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT4A() {
	FZ(A & 0x10);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT4M() {
	FZ(cpuBus->Read((H << 8) | L) & 0x10);
	mTemp = 3; tTemp = 12;
}

void CPU::BIT5B() {
	FZ(B & 0x20);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT5C() {
	FZ(C & 0x20);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT5D() {
	FZ(D & 0x20);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT5E() {
	FZ(E & 0x20);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT5H() {
	FZ(H & 0x20);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT5L() {
	FZ(L & 0x20);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT5A() {
	FZ(A & 0x20);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT5M() {
	FZ(cpuBus->Read((H << 8) | L) & 0x20);
	mTemp = 3; tTemp = 12;
}

void CPU::BIT6B() {
	FZ(B & 0x40);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT6C() {
	FZ(C & 0x40);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT6D() {
	FZ(D & 0x40);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT6E() {
	FZ(E & 0x40);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT6H() {
	FZ(H & 0x40);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT6L() {
	FZ(L & 0x40);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT6A() {
	FZ(A & 0x40);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT6M() {
	FZ(cpuBus->Read((H << 8) | L) & 0x40);
	mTemp = 3; tTemp = 12;
}

void CPU::BIT7B() {
	FZ(B & 0x80);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT7C() {
	FZ(C & 0x80);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT7D() {
	FZ(D & 0x80);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT7E() {
	FZ(E & 0x80);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT7H() {
	FZ(H & 0x80);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT7L() {
	FZ(L & 0x80);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT7A() {
	FZ(A & 0x80);
	mTemp = 2; tTemp = 8;
}

void CPU::BIT7M() {
	FZ(cpuBus->Read((H << 8) | L) & 0x80);
	mTemp = 3; tTemp = 12;
}

void CPU::RES0A() {
	A &= 0xFE;
	mTemp = 2;
	tTemp = 8;
}

#pragma endregion


void CPU::Reset() {
	cpuBus->inBios = false;
	A = 0x1;
	B = 0x0;
	C = 0x13;
	D = 0x0;
	E = 0xD8;
	F = 0xB0;
	H = 0x1;
	L = 0x4D;
	SP = 0xFFFE;
	PC = 0x101;
	IME = 1;
	cpuBus->SetIF(0xE1);
	cpuBus->SetIE(0);
	gpu->Reset();
	mClock = 0; tClock = 0;
	halt = false; stop = false;
}

void CPU::Update() {
	if (halt) {
		mTemp = 1; tTemp = 4;
	}
	else {
		(this->*instructions[cpuBus->Read(PC++)].op)();
		F &= 0xF0;
		PC &= 0xFFFF;
	}
	int ieT, ifT;
	ieT = cpuBus->GetIE();
	ifT = cpuBus->GetIF();
	if (IME && ieT && ifT) { 
		halt = 0; IME = 0;
		bool ifired = ieT & ifT;
		if (ifired & 1) {
			ifT &= 0xFE;
			cpuBus->SetIF(ifT);
			RST40();
		}
		else if (ifired & 2) {
			ifT &= 0xFD;
			cpuBus->SetIF(ifT);
			RST48();
		}
		else if (ifired & 4) {
			ifT &= 0xFB;
			cpuBus->SetIF(ifT);
			RST50();
		}
		else if (ifired & 8) {
			ifT &= 0xF7;
			cpuBus->SetIF(ifT);
			RST58();
		}
		else if (ifired & 16) {
			ifT &= 0xEF;
			cpuBus->SetIF(ifT);
			RST60();
		}
		else {
			IME = 1;
		}
	}
	gpu->Update(mTemp);
	mClock += mTemp;
	tClock += tTemp;
}