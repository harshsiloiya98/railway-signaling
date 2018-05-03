----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    19:57:48 03/18/2018 
-- Design Name: 
-- Module Name:    gen_encrypted - Behavioral 
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

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx primitives in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity gen_encrypted is
    Port ( clock : in  STD_LOGIC;
			  reset : in  STD_LOGIC;
			  enable : in  STD_LOGIC;
			  coordinates : in  STD_LOGIC_VECTOR (31 downto 0);
           coordinates_enc : out  STD_LOGIC_VECTOR (31 downto 0);
           ack1 : in  STD_LOGIC_VECTOR (31 downto 0);
           ack1_enc : out  STD_LOGIC_VECTOR (31 downto 0);
           ack2 : in  STD_LOGIC_VECTOR (31 downto 0);
           ack2_enc : out  STD_LOGIC_VECTOR (31 downto 0);
			  done : out STD_LOGIC);
end gen_encrypted;

architecture Behavioral of gen_encrypted is

signal P_enc : std_logic_vector(31 downto 0) := (others => '0');
signal C_enc : std_logic_vector(31 downto 0) := (others => '0');
signal K : std_logic_vector(31 downto 0) := "11001100110011001100110011000001";
signal reset_enc : std_logic;
signal enable_enc : std_logic;
signal done_enc : std_logic;
signal state : std_logic_vector(2 downto 0) := (others => '0');

component encrypter is
    Port ( clock : in  STD_LOGIC;
           K : in  STD_LOGIC_VECTOR (31 downto 0);
           P : in  STD_LOGIC_VECTOR (31 downto 0);
           C : out  STD_LOGIC_VECTOR (31 downto 0);
           reset : in  STD_LOGIC;
           enable : in  STD_LOGIC;
           done : out STD_LOGIC);
end component;

begin
	enc: encrypter port map(clock, K, P_enc, C_enc, reset_enc, enable_enc, done_enc);
	process(clock, reset, enable)
	begin
		if (reset = '1') then
			coordinates_enc <= (others => '0');
			ack1_enc <= (others => '0');
			ack2_enc <= (others => '0');
			done <= '0';
		elsif (clock'event and clock = '1' and enable = '1') then
			
			-- encrypt coordinates reset
			if (state = "000") then
				P_enc <= coordinates;
				reset_enc <= '1';
				enable_enc <= '1';
				state <= "001";
			end if;

			-- encrypt coordinates
			if (state = "001") then
				if (done_enc = '1') then
					state <= "010";
					coordinates_enc <= C_enc;
				else
					reset_enc <= '0';
				end if;
			end if;

			-- encrypt ack1 reset
			if (state = "010") then
				P_enc <= ack1;
				reset_enc <= '1';
				enable_enc <= '1';
				state <= "011";
			end if;

			-- encrypt ack1
			if (state = "011") then
				if (done_enc = '1') then
					state <= "100";
					ack1_enc <= C_enc;
				else
					reset_enc <= '0';
				end if;
			end if;
			
			-- encrypt ack2 reset
			if (state = "100") then
				P_enc <= ack2;
				reset_enc <= '1';
				enable_enc <= '1';
				state <= "101";
			end if;

			-- encrypt coordinates
			if (state = "101") then
				if (done_enc = '1') then
					ack2_enc <= C_enc;
					done <= '1';
				else
					reset_enc <= '0';
				end if;
			end if;
		end if;
	end process;
end Behavioral;

