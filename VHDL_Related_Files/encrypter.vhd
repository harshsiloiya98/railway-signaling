----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    14:54:52 02/06/2018 
-- Design Name: 
-- Module Name:    encrypter - Behavioral 
-- Project Name: 
-- Target Devices: 
-- Tool versions: 
-- Description: 
--
-- Dependencies: 
--
-- Revision: 
-- Revision 0.01 - File Created
-- Additional Comments: 
--
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity encrypter is
    Port ( clock : in  STD_LOGIC;
           K : in  STD_LOGIC_VECTOR (31 downto 0);
           P : in  STD_LOGIC_VECTOR (31 downto 0);
           C : out  STD_LOGIC_VECTOR (31 downto 0);
           reset : in  STD_LOGIC;
           enable : in  STD_LOGIC;
           done : out STD_LOGIC);
end encrypter;

architecture Behavioral of encrypter is

signal N1 : std_logic_vector(5 downto 0) := "000000";
signal counter : std_logic_vector(5 downto 0) := "000000";
signal dC : std_logic_vector(31 downto 0);
signal T : std_logic_vector(3 downto 0) := "0000";
signal i : std_logic_vector(5 downto 0) := "000000";
begin
   process(clock, reset, enable)
	 begin
		if (reset = '1') then
		-- Initializing all signals to 0. Reset is needed when modifying input from testbench for iterators to work properly.
			C <= "00000000000000000000000000000000";
			N1 <= "000000";
			i <= "000000";
			counter <= "000000";
			done <= '0';
		elsif (clock'event and clock = '1' and enable = '1') then
			if counter = 0 then
			-- In the first step, we use a dummy signal dC to keep track of the value of C.
			-- Also, Initial computation of T is done.
				dC <= P;
				T(3) <= K(31) xor K(27) xor K(23) xor K(19) xor K(15) xor K(11) xor K(7) xor K(3);
				T(2) <= K(30) xor K(26) xor K(22) xor K(18) xor K(14) xor K(10) xor K(6) xor K(2);
				T(1) <= K(29) xor K(25) xor K(21) xor K(17) xor K(13) xor K(09) xor K(5) xor K(1);
				T(0) <= K(28) xor K(24) xor K(20) xor K(16) xor K(12) xor K(08) xor K(4) xor K(0);
			end if;
			if counter < 32 then
			-- Counting the number of 1's in K. For each bit of K equal to 1, N1 gets incremented by 1.
			-- Number of iterations of counter is 32 (Since K is a 32-bit number).
				N1 <= N1 + K(to_integer(unsigned(counter)));
				counter <= counter + 1;
			else
				-- Condition ensuring 'i' iterates from 0 to N1-1.
				if i < N1 then
					-- Concatenating T to itself 8 times and then XORing with dC(dummy signal for C).
					dC <= dC xor (T & T & T & T & T & T & T & T);
					T <= T + 1;
					i <= i + 1;
				elsif i = N1 then
					-- Once the above procedure is complete, we assign to C it's dummy value (stored in dC).
					C <= dC;
					-- Incrementing 'i' so that there is no unnecessary assignment again and again.
					i <= i + 1;
					done <= '1';
				end if;			
			end if;
		end if;	
	 end process;
end Behavioral;