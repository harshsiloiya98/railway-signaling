----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    22:21:02 03/18/2018 
-- Design Name: 
-- Module Name:    gen_decrypted - Behavioral 
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
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx primitives in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity gen_decrypted is
    Port ( clock : in  STD_LOGIC;
			  reset : in  STD_LOGIC;
           enable : in  STD_LOGIC;
			  inp_direction : in  STD_LOGIC_VECTOR (63 downto 0);
           direction_data : out  STD_LOGIC_VECTOR (63 downto 0);
			  done : out STD_LOGIC);
end gen_decrypted;

architecture Behavioral of gen_decrypted is

signal state : std_logic := '0';
signal part : std_logic_vector(1 downto 0) := "00";
signal P_dec : std_logic_vector(31 downto 0) := (others => '0');
signal C_dec : std_logic_vector(31 downto 0) := (others => '0');
signal reset_dec : std_logic;
signal enable_dec : std_logic;
signal done_dec : std_logic;
signal K : std_logic_vector(31 downto 0) := "11001100110011001100110011000001";

component decrypter is
    Port ( clock : in  STD_LOGIC;
           K : in  STD_LOGIC_VECTOR (31 downto 0);
           C : in  STD_LOGIC_VECTOR (31 downto 0);
           P : out  STD_LOGIC_VECTOR (31 downto 0);
           reset : in  STD_LOGIC;
           enable : in  STD_LOGIC;
           done : out STD_LOGIC);
end component;

begin
	dec: decrypter port map(clock, K, C_dec, P_dec, reset_dec, enable_dec, done_dec);
	process(clock, reset, enable)
	begin
		if (reset = '1') then
			direction_data <= (others => '0');
			done <= '0';
		elsif (clock'event and clock = '1' and enable = '1') then
			if (state = '0') then
				if (part = "10") then
					done <= '1';
				else
					C_dec <= inp_direction((to_integer(unsigned(part))*32 + 31) downto to_integer(unsigned(part))*32);
					reset_dec <= '1';
					enable_dec <= '1';
					state <= '1';
				end if;
			end if;

			-- decrypt coordinates
			if (state = '1') then
				if (done_dec = '1') then
					state <= '0';
					direction_data((to_integer(unsigned(part))*32 + 31) downto to_integer(unsigned(part))*32) <= P_dec;
					part <= part + 1;
				else
					reset_dec <= '0';
				end if;
			end if;
		end if;
	end process;
end Behavioral;

