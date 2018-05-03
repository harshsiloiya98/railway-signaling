----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    18:35:53 03/15/2018 
-- Design Name: 
-- Module Name:    timeout - Behavioral 
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

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx primitives in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity timeout is
    Port ( reset : in  STD_LOGIC;
           enable : in  STD_LOGIC;
           done : out  STD_LOGIC;
           clock : in  STD_LOGIC);
end timeout;

architecture Behavioral of timeout is

signal sec_counter : std_logic_vector(26 downto 0) := (others => '0');
signal TO_counter : std_logic_vector(7 downto 0) := (others => '0');

begin
	process(clock, reset, enable)
		begin
			if (reset = '1') then
				sec_counter <= (others => '0');
				TO_counter <= (others => '0');
				done <= '0';
			elsif (clock'event and clock = '1' and enable = '1') then
				if (TO_counter = "11111111") then
					done <= '1';
				end if;
				if (to_integer(unsigned(sec_counter)) = 100000000) then
					sec_counter <= (others => '0');
					TO_counter <= TO_counter + 1;
				else
					sec_counter <= sec_counter + 1;
				end if;
			end if;
	end process;

end Behavioral;

