----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    11:15:53 03/19/2018 
-- Design Name: 
-- Module Name:    direction_data - Behavioral 
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

entity direction_data is
    Port ( clock : in  STD_LOGIC;
           reset : in  STD_LOGIC;
           enable : in  STD_LOGIC;
           input_direction : in  STD_LOGIC_VECTOR (63 downto 0);
           output : out  STD_LOGIC_VECTOR (63 downto 0);
           done : out  STD_LOGIC);
end direction_data;

architecture Behavioral of direction_data is

signal count : std_logic_vector(3 downto 0) := "0000";
signal input_temp : std_logic_vector(7 downto 0) := "00000000";
signal output_temp : std_logic_vector(7 downto 0) := "00000000";
signal direction_reset : std_logic;
signal direction_output : std_logic;
signal state : std_logic_vector(1 downto 0) := "00";

begin
	process(clock, reset, enable)
	begin
		if (reset = '1') then
			output <= (others => '0');
			count <= (others => '0');
			done <= '0';
			direction_reset <= '1';
			direction_output <= '0';
		elsif (clock'event and clock = '1' and enable = '1') then
			if (count = "1000") then
				done <= '1';
			else
				if (state = "00") then
					state <= "01";
					output(to_integer(unsigned(count))*8+7 downto (to_integer(unsigned(count)))*8+5) <= input_direction(to_integer(unsigned(count))*8+5 downto (to_integer(unsigned(count)))*8+3);
				elsif (state = "01") then
					if (input_direction(to_integer(unsigned(count))*8+7 downto (to_integer(unsigned(count)))*8+6) = "11") then
						if (input_direction(to_integer(unsigned(count))*8+2 downto (to_integer(unsigned(count)))*8+1) = "00") then
							output(to_integer(unsigned(count))*8+1) <= '1';
						else
							output(to_integer(unsigned(count))*8) <= '1';
						end if;
					else
						output(to_integer(unsigned(count))*8+2) <= '1';
					end if;
					state <= "10";
				else
					count <= count + 1;
					state <= "00";
				end if;
			end if;
		end if;
	end process;
end Behavioral;