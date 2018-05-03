----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    12:28:11 03/19/2018 
-- Design Name: 
-- Module Name:    board_data - Behavioral 
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
use ieee.numeric_std.all;
use ieee.std_logic_unsigned.all;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx primitives in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity board_data is
    Port ( clock : in  STD_LOGIC;
           reset : in  STD_LOGIC;
           enable : in  STD_LOGIC;
           input_direction : in  STD_LOGIC_VECTOR (63 downto 0);
           input_board : in  STD_LOGIC_VECTOR (7 downto 0);
           output : out  STD_LOGIC_VECTOR (191 downto 0);
			  done : out  STD_LOGIC);
end board_data;

architecture Behavioral of board_data is

signal count : std_logic_vector(3 downto 0) := "0000";
signal state : std_logic_vector(1 downto 0) := "00";
signal part : std_logic_vector(1 downto 0) := "00";

begin
	process(clock, reset, enable)
	begin
		if (reset = '1') then
			output <= (others => '0');
			count <= (others => '0');
			done <= '0';
			state <= "00";
			part <= "00";
		elsif (clock'event and clock = '1' and enable = '1') then
			if (count = "1000") then
				done <= '1';
			else
				if (state = "00") then
					state <= "01";
					output((to_integer(unsigned(count))*3 + to_integer(unsigned(part)))*8+7 downto ((to_integer(unsigned(count))*3 + to_integer(unsigned(part)))*8+5)) <= input_direction(to_integer(unsigned(count))*8+5 downto (to_integer(unsigned(count)))*8+3);
				elsif (state = "01") then
					state <= "10";
					if (input_direction(to_integer(unsigned(count))*8+6) = '0') then
						output((to_integer(unsigned(count))*3 + to_integer(unsigned(part)))*8+2) <= '1';
					else
						if to_integer(unsigned(input_direction(to_integer(unsigned(count))*8+2 downto to_integer(unsigned(count))*8))) < 2 then
							output((to_integer(unsigned(count))*3 + to_integer(unsigned(part)))*8+1) <= '1';
						else
							if (input_board(to_integer(unsigned(count))) = '0') then
								output((to_integer(unsigned(count))*3 + to_integer(unsigned(part)))*8+2) <= '1';
							else
								if (input_board((to_integer(unsigned(count))+4) mod 8) = '0') then
									output((to_integer(unsigned(count))*3 + to_integer(unsigned(part)))*8) <= '1';
								else
									if (to_integer(unsigned(count)) > ((to_integer(unsigned(count))+4) mod 8)) then
										output((to_integer(unsigned(count))*3 + to_integer(unsigned(part)))*8+2) <= '1';
									else
										if (part = "00") then
											output((to_integer(unsigned(count))*3 + to_integer(unsigned(part)))*8+2) <= '1';
										elsif (part = "01") then
											output((to_integer(unsigned(count))*3 + to_integer(unsigned(part)))*8+1) <= '1';
										else
											output((to_integer(unsigned(count))*3 + to_integer(unsigned(part)))*8) <= '1';
										end if;
									end if;
								end if;
							end if;
						end if;
					end if;
					if (part = "10") then
						state <= "11";
					else
						state <= "10";
					end if;
				elsif (state = "10") then
					part <= part + 1;
					state <= "00";
				else
					part <= "00";
					count <= count + 1;
					state <= "00";
				end if;
			end if;
		end if;
	end process;
end Behavioral;