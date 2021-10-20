#include <Arduino.h>
#include "hp35Calc.h"

#include "hp35Rom.h"

#define WSIZE 14
#define ZERO 0

HPCalc::HPCalc() {
   //implementation
}

// public...

String HPCalc::getResult() {
   return HPCalc::resultString; // return the string here...
}

void enterCommand(int command) {
  // do stuff....
}

// private...

void HPCalc::buildResultString() {
  HPCalc::resultString = "get bent";
  for (int i = WSIZE - 1; i >= 0; i--) {
    if (b [i] >= 8) {
      // print_lcd(' ');
      HPCalc::resultString.concat(" ");
    } else if (i == 2) {
		  if (a [i] >= 8) {
	      // print_lcd('-');
        HPCalc::resultString.concat("-");
      } else {
        //  print_lcd(' ');
        HPCalc::resultString.concat(" ");
      }
	  } else if (i == 13) {
      if (a [i] >= 8) {
        // print_lcd('-');
        HPCalc::resultString.concat("-");
      } else {
        // print_lcd(' ');
        HPCalc::resultString.concat(" ");
	    }
	  } else {
      // print_lcd(a[i]+48); // ascii ?
      HPCalc::resultString.concat(a[i]+48);
 	  }
	  if (b [i] == 2) {
	    // print_lcd('.');
      HPCalc::resultString.concat(" ");
    }	  
   }
}

byte HPCalc::do_add(byte x, byte y) { // Add 2 bytes
  int res = x + y + carry;
  if (res > 9) {
    res -= 10;
    carry = 1;
  }
  else carry = 0;
  return ((byte) res);
}

byte HPCalc::do_sub(byte x, byte y)  { // Substract 2 bytes
  int res = x - y - carry;
  if (res < 0) {
    res += 10;
    carry = 1;
  }
  else carry = 0;
  return ((byte)res);
}

void HPCalc::process_rom(void) { // Process key with HP35-engine
  if ((pc == 191) & (offset == 0)) { // *** Error handling
    isError = true;
    //Serial.println("Error");
  }

  prevCarry = carry; // *** Fetch ROM
  carry = 0;
  fetch_h = pgm_read_byte_near(hp35Rom + (offset * 256 * 2) + (pc * 2));
  fetch_l = pgm_read_byte_near(hp35Rom + (offset * 256 * 2) + (pc * 2) + 1);
  //fetch_h = rom [(offset * 256 * 2) + (pc * 2)];
  //fetch_l = rom [(offset * 256 * 2) + (pc * 2) + 1];
  pc++;
  pc %= 256;

  if (key_code < 255) { // *** Process received key
    isError=false;
    key_rom = key_code;
    key_code = 255;
    s[0] = 1;
  }

  if (fetch_l == 0x00) ; // NOP  *** Evaluate fetch
  if ((fetch_l & 0x03) == 0x01) { // Jump subroutine
    ret = pc;
    pc = (((fetch_l >> 2) & 0x3f) | ((fetch_h << 6) & 0x0c0));
  }
  if ((fetch_l & 0x7F) == 0x030) pc = ret; // Return from subroutine
  if ((fetch_l & 0x7F) == 0x010) offset = (((fetch_h << 1) & 06) | ((fetch_l >> 7) & 01)); // ROM
  if (fetch_l == 0x0D0) { // Jump to pc + rom if key is available
    pc = key_rom; // Reset pointer
    s[0] = 0;
  }
  if ((fetch_l & 0x03f) == 0x014) // Set carry due to status
    carry = s[((fetch_h & 0x03) << 2) | ((fetch_l & 0x0c0) >> 6)];
  if ((fetch_l & 0x03f) == 0x004) s[((fetch_h & 0x03) << 2) | ((fetch_l & 0x0c0) >> 6)] = 1; // Set s
  if ((fetch_l & 0x03f) == 0x024) s[((fetch_h & 0x03) << 2) | ((fetch_l & 0x0c0) >> 6)] = 0; // Clear s
  if ((fetch_l & 0x03f) == 0x034) for (byte i = 0; i < 12; i++) s[i] = 0; // Clear status
  if ((fetch_l & 0x03f) == 0x02C) { // Set carry
    carry = 0;
    if (p == (((fetch_h & 0x03) << 2) | ((fetch_l & 0x0c0) >> 6))) carry = 1;
  }
  if ((fetch_l & 0x03f) == 0x00C) p = (((fetch_h & 0x03) << 2) | ((fetch_l & 0x0c0) >> 6)); // Set p
  if ((fetch_l & 0x03f) == 0x03C) p = ((p + 1) & 0x0f);
  if ((fetch_l & 0x03f) == 0x01C) p = ((p - 1) & 0x0f);
  if ((fetch_l & 0x3F) == 0x18) { // Load constant
    c[p] = ((fetch_l >> 6) | (fetch_h << 2));
    p = ((p - 1) & 0x0f);
  }
  if (((fetch_h & 0x03) == 0x00) && ((fetch_l & 0x0ef) == 0x0A8)) { // c<->m
    for (byte i = 0; i < WSIZE; i++) {
      int tmp = c[i];
      c[i] = m[i];
      m[i] = tmp;
    }
  }
  if (((fetch_h & 0x03) == 0x01) && ((fetch_l & 0x0ef) == 0x028)) { // c to stack
    for (byte i = 0; i < WSIZE; i++) {
      f[i] = e[i];
      e[i] = d[i];
      d[i] = c[i];
    }
  }
  if (((fetch_h & 0x03) == 0x01) && ((fetch_l & 0x0ef) == 0x0A8)) { // Stack to a
    for (byte i = 0; i < WSIZE; i++) {
      a[i] = d[i];
      d[i] = e[i];
      e[i] = f[i];
    }
  }
  if (((fetch_h & 0x03) == 0x02) && ((fetch_l & 0x0ef) == 0x0A8)) { // m to c
    for (byte i = 0; i < WSIZE; i++) c[i] = m[i];
  }
  if (((fetch_h & 0x03) == 0x03) && ((fetch_l & 0x0ef) == 0x028)) { // Rotate down
    for (byte i = 0; i < WSIZE; i++) {
      int tmp = c[i];
      c[i] = d[i];
      d[i] = e[i];
      e[i] = f[i];
      f[i] = tmp;
    }
  }
  if (((fetch_h & 0x03) == 0x03) && ((fetch_l & 0x0ef) == 0x0A8)) // Clear all register
    for (byte i = 0; i < WSIZE; i++) a[i] = b[i] = c[i] = d[i] = e[i] = f[i] = m[i] = 0;
  if (((fetch_h & 0x03) == 0x00) && ((fetch_l & 0x0ef) == 0x028)) { // No display
    display_enable = false;
    update_display =  false;
  }
  if (((fetch_h & 0x03) == 0x02) && ((fetch_l & 0x0ef) == 0x028)) { // Toggle display
    display_enable = ! display_enable;
    if (display_enable == true) update_display = true;
  }
  if ((fetch_l & 0x03) == 0x03) // Conditional branch
    if ( prevCarry != 1) pc = ((fetch_l & 0x0fc) >> 2) | ((fetch_h & 0x03) << 6);
  if ((fetch_l & 0x03) == 0x02) { // A&R calculations due to opcode
    op_code = ((fetch_l >> 5) & 0x07);
    op_code = op_code | ((fetch_h << 3) & 0x018);
    switch ((fetch_l >> 2) & 0x07) { // Get register boundaries first/last
      case 0:
        first = last = p;
        break;
      case 1:
        first = 3;
        last = 12;
        break;
      case 2:
        first = 0;
        last = 2;
        break;
      case 3:
        first = 0;
        last = 13;
        break;
      case 4:
        first = 0;
        last = p;
        break;
      case 5:
        first = 3;
        last = 13;
        break;
      case 6:
        first = last = 2;
        break;
      case 7:
        first = last = 13;
        break;
    }
    carry = 0;
    switch (op_code) { // Process opcode
      case 0x0: // 0+B
        for (byte i = first; i <= last; i++) if (b[i] != 0) carry = 1;
        break;
      case 0x01: // 0->B
        for (byte i = first; i <= last; i++) b[i] = 0;
        break;
      case 0x02: //  A-C
        for (byte i = first; i <= last; i++) t[i] = do_sub(a[i], c[i]);
        break;
      case 0x03: // C-1
        carry = 1;
        for (byte i = first; i <= last; i++) if (c[i] != 0) carry = 0;
        break;
      case 0x04: // B->C
        for (byte i = first; i <= last; i++) c[i] = b[i];
        break;
      case 0x05: // 0-C->C
        for (byte i = first; i <= last; i++) c[i] = do_sub(ZERO, c[i]);
        break;
      case 0x06: // 0->C
        for (byte i = first; i <= last; i++) c[i] = 0;
        break;
      case 0x07: // 0-C-1->C
        carry = 1;
        for (byte i = first; i <= last; i++) c[i] = do_sub(ZERO, c[i]);
        break;
      case 0x08: // Sh A Left
        for (int8_t i = last; i > first; i--) a[i] = a[i - 1];
        a[first] = 0;
        break;
      case 0x09: // A->B
        for (byte i = first; i <= last; i++) b[i] = a[i];
        break;
      case 0x0a: // A-C->C
        for (byte i = first; i <= last; i++) c[i] = do_sub(a[i], c[i]);
        break;
      case 0x0b: // C-1->C
        carry = 1;
        for (byte i = first; i <= last; i++) c[i] = do_sub(c[i], ZERO);
        break;
      case 0x0c: // C->A
        for (byte i = first; i <= last; i++) a[i] = c[i];
        break;
      case 0x0d: // 0-C
        for (byte i = first; i <= last; i++) if (c[i] != 0) carry = 1;
        break;
      case 0x0e: // A+C->C
        for (byte i = first; i <= last; i++) c[i] = do_add(a[i], c[i]);
        break;
      case 0x0f: // C+1->C
        carry = 1;
        for (byte i = first; i <= last; i++) c[i] = do_add(c[i], ZERO);
        break;
      case 0x010: // A-B
        for (byte i = first; i <= last; i++) t[i] = do_sub(a[i], b[i]);
        break;
      case 0x011: // B<->C
        for (byte i = first; i <= last; i++) {
          byte tmp = b[i];
          b[i] = c[i];
          c[i] = tmp;
        }
        break;
      case 0x012: // Sh C Right
        for (byte i = first; i < last; i++) c[i] =  c[i + 1];
        c[last] = 0;
        break;
      case 0x013: // A-1
        carry = 1;
        for (byte i = first; i <= last; i++) if (a[i] != 0) carry = 0;
        break;
      case 0x014: // Sh B Right
        for (byte i = first; i < last; i++) b[i] =  b[i + 1];
        b[last] = 0;
        break;
      case 0x015: // C+C->A
        for (byte i = first; i <= last; i++) c[i] = do_add(c[i], c[i]);
        break;
      case 0x016: // Sh A Right
        for (byte i = first; i < last; i++) a[i] = a[i + 1];
        a[last] = 0;
        break;
      case 0x017: // 0->A
        for (byte i = first; i <= last; i++) a[i] = 0;
        break;
      case 0x018: // A-B->A
        for (byte i = first; i <= last; i++) a[i] = do_sub(a[i], b[i]);
        break;
      case 0x019: // A<->B
        for (byte i = first; i <= last; i++) {
          byte tmp = a[i];
          a[i] = b[i];
          b[i] = tmp;
        }
        break;
      case 0x01a: // A-C->A
        for (byte i = first; i <= last; i++) a[i] = do_sub(a[i], c[i]);
        break;
      case 0x01b: // A-1->A
        carry = 1;
        for (byte i = first; i <= last; i++) a[i] = do_sub(a[i], ZERO);
        break;
      case 0x01c: // A+B->A
        for (byte i = first; i <= last; i++) a[i] = do_add(a[i], b[i]);
        break;
      case 0x01d: // C<->A
        for (byte i = first; i <= last; i++) {
          byte tmp = a[i];
          a[i] = c[i];
          c[i] = tmp;
        }
        break;
      case 0x01e: // A+C->A
        for (byte i = first; i <= last; i++) a[i] = do_add(a[i], c[i]);
        break;
      case 0x01f: // A+1->A
        carry = 1;
        for (byte i = first; i <= last; i++) a[i] = do_add(a[i], ZERO);
        break;
    }
  }

  if (display_enable) { // *** Display
    update_display = true;
  }
  else if (update_display) {
   //  lcd_HP35();
    update_display = false;
  }
}