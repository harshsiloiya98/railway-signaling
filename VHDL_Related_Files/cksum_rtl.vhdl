----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    15:30:39 02/20/2018 
-- Design Name: 
-- Module Name:    Rail_Signaling - Behavioral 
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

--entity Rail_Signaling is
--end Rail_Signaling;

architecture rtl of swled is

-- unused
signal flags                   : std_logic_vector(3 downto 0);
signal checksum, checksum_next : std_logic_vector(15 downto 0) := (others => '0');
	
-- board controller
signal reset : std_logic := '1';
constant enable : std_logic := '1';
signal Mstate : std_logic_vector(2 downto 0) := "001";
signal state : std_logic_vector(4 downto 0) := (others => '0');
signal display_state : std_logic_vector(2 downto 0) := (others => '0');

-- input
signal out_reg : std_logic_vector(7 downto 0)  := (others => '0');
signal inp_reg, inp_reg_next : std_logic_vector(7 downto 0)  := (others => '0');

-- comm
constant channel_in : std_logic_vector(6 downto 0) := "0000001";
constant channel_out : std_logic_vector(6 downto 0) := "0000000";
signal coordinates_receive : std_logic_vector(31 downto 0) := (others => '0');
signal ack2_received : std_logic_vector(31 downto 0) := (others => '0');

-- led out
signal L : std_logic_vector(7 downto 0) := (others => '0');
signal sec_counter : std_logic_vector(26 downto 0) := (others => '0');
signal led_counter : std_logic_vector(4 downto 0) := (others => '0');
signal wait_counter : std_logic_vector(1 downto 0) := (others => '0');
	

signal reset_enc : std_logic := '1';
constant enable_enc : std_logic := '1';
signal done_enc : std_logic;
constant coordinates : std_logic_vector(31 downto 0) := "00000000000000000000000000110110";
signal coordinates_enc : std_logic_vector(31 downto 0);
constant ack1 : std_logic_vector(31 downto 0) := "01010101010101010101010101010101";
constant ack2 : std_logic_vector(31 downto 0) := "10101010101010101010101010101010";
signal ack1_enc : std_logic_vector(31 downto 0);
signal ack2_enc : std_logic_vector(31 downto 0);
component gen_encrypted is
    Port (  clock : in  STD_LOGIC;
			reset : in  STD_LOGIC;
			enable : in  STD_LOGIC;
			coordinates : in  STD_LOGIC_VECTOR (31 downto 0);
            coordinates_enc : out  STD_LOGIC_VECTOR (31 downto 0);
           	ack1 : in  STD_LOGIC_VECTOR (31 downto 0);
           	ack1_enc : out  STD_LOGIC_VECTOR (31 downto 0);
           	ack2 : in  STD_LOGIC_VECTOR (31 downto 0);
           	ack2_enc : out  STD_LOGIC_VECTOR (31 downto 0);
			done : out STD_LOGIC);
end component;

signal reset_dec : std_logic := '1';
constant enable_dec : std_logic := '1';
signal done_dec : std_logic;
signal inp_direction : std_logic_vector(63 downto 0) := (others => '0');
signal direc_data : std_logic_vector(63 downto 0);
component gen_decrypted is
    Port ( clock : in  STD_LOGIC;
		   reset : in  STD_LOGIC;
           enable : in  STD_LOGIC;
		   inp_direction : in  STD_LOGIC_VECTOR (63 downto 0);
           direction_data : out  STD_LOGIC_VECTOR (63 downto 0);
		   done : out STD_LOGIC);
end component;

signal reset_TO : std_logic := '1';
constant enable_TO : std_logic := '1';
signal done_TO : std_logic;
component timeout is
    Port ( reset : in  STD_LOGIC;
           enable : in  STD_LOGIC;
           done : out  STD_LOGIC;
           clock : in  STD_LOGIC);
end component;

signal reset_direction : std_logic := '1';
constant enable_direction : std_logic := '1';
signal done_direction : std_logic;
signal output_result : std_logic_vector(63 downto 0);
component direction_data is
   Port ( clock : in  STD_LOGIC;
           reset : in  STD_LOGIC;
           enable : in  STD_LOGIC;
           input_direction : in  STD_LOGIC_VECTOR (63 downto 0);
           output : out  STD_LOGIC_VECTOR (63 downto 0);
           done : out  STD_LOGIC);
end component;

signal reset_board : std_logic := '1';
constant enable_board : std_logic := '1';
signal done_board : std_logic;
signal board_output : std_logic_vector(191 downto 0);
signal data_output: std_logic_vector(191 downto 0);
signal board_input : std_logic_vector(7 downto 0) := (others => '0');
component board_data is
    Port ( clock : in  STD_LOGIC;
           reset : in  STD_LOGIC;
           enable : in  STD_LOGIC;
           input_direction : in  STD_LOGIC_VECTOR (63 downto 0);
           input_board : in  STD_LOGIC_VECTOR (7 downto 0);
           output : out  STD_LOGIC_VECTOR (191 downto 0);
		   done : out  STD_LOGIC);
end component;

signal reset_encrypter : std_logic := '1';
constant enable_encrypter : std_logic := '1';
signal done_encrypter : std_logic;
constant K : std_logic_vector(31 downto 0) := "11001100110011001100110011000001";
signal fpga_in : std_logic_vector(31 downto 0) := (others => '0');
signal fpga_enc : std_logic_vector(31 downto 0);
component encrypter is
    Port ( clock : in  STD_LOGIC;
           K : in  STD_LOGIC_VECTOR (31 downto 0);
           P : in  STD_LOGIC_VECTOR (31 downto 0);
           C : out  STD_LOGIC_VECTOR (31 downto 0);
           reset : in  STD_LOGIC;
           enable : in  STD_LOGIC;
           done : out STD_LOGIC);
end component;

signal output_count : std_logic_vector (2 downto 0) := (others => '0');
signal reset_transmission : std_logic := '1';
constant enable_transmission : std_logic := '1';
signal trans_state : std_logic := '0';

signal input_count : std_logic_vector (2 downto 0) := (others => '0');
signal reset_reception : std_logic := '1';
constant enable_reception : std_logic := '1';
signal recep_state : std_logic := '0';

signal count : std_logic_vector(3 downto 0) := "0000";
signal board_state : std_logic_vector(1 downto 0) := "00";
signal board_part : std_logic_vector(1 downto 0) := "00";

signal upcheck : std_logic := '0';
signal downcheck : std_logic := '0';
signal leftcheck : std_logic := '0';
signal rightcheck : std_logic := '0';
signal resetcheck : std_logic := '0';

signal uppressed : std_logic := '0';
signal downpressed : std_logic := '0';
signal leftpressed : std_logic := '0';
signal rightpressed : std_logic := '0';

signal UART_share : std_logic_vector(7 downto 0) := (others => '0');
signal UART_received : std_logic_vector(7 downto 0) := "10011001";
signal UART_rec : std_logic_vector(7 downto 0);
component debouncer
        port(clk: in STD_LOGIC;
            button: in STD_LOGIC;
            button_deb: out STD_LOGIC);
end component;

signal reset_t : std_logic := '1';

component uart_tx is
  generic (
    g_CLKS_PER_BIT : integer := 434     -- Needs to be set correctly
    );
  port (
    i_Clk       : in  std_logic;
    i_TX_DV     : in  std_logic;
    i_TX_Byte   : in  std_logic_vector(7 downto 0);
    o_TX_Active : out std_logic;
    o_TX_Serial : out std_logic;
    o_TX_Done   : out std_logic
    );
end component;
signal tx_active: std_logic;
signal tx_serial: std_logic;
signal tx_done: std_logic;

component uart_rx is
  generic (
    g_CLKS_PER_BIT : integer := 434     -- Needs to be set correctly
    );
  port (
    i_Clk       : in  std_logic;
    i_RX_Serial : in  std_logic;
    o_RX_DV     : out std_logic;
    o_RX_Byte   : out std_logic_vector(7 downto 0)
    );
end component;
signal r_TX_DV     : std_logic;
signal r_TX_BYTE   : std_logic_vector(7 downto 0) := (others => '0');
signal w_TX_SERIAL : std_logic;
signal w_TX_DONE   : std_logic;
signal w_RX_DV     : std_logic;
signal w_RX_BYTE   : std_logic_vector(7 downto 0);
signal r_RX_SERIAL : std_logic := '1';

signal uart_data_received : std_logic;
signal uart_data_to_be_used : std_logic := '0';

begin
	debouncer1: debouncer
              port map (clk => clk_in,
                        button => up_b,
                        button_deb => upcheck);

	debouncer2: debouncer
              port map (clk => clk_in,
                        button => down_b,
                        button_deb => downcheck);

	debouncer3: debouncer
              port map (clk => clk_in,
                        button => left_b,
                        button_deb => leftcheck);

	debouncer4: debouncer
              port map (clk => clk_in,
                        button => right_b,
                        button_deb => rightcheck);

	debouncer5: debouncer
              port map (clk => clk_in,
                        button => reset_b,
                        button_deb => resetcheck);

	enc: gen_encrypted port map (clk_in, reset_enc, enable_enc, coordinates, coordinates_enc, ack1, ack1_enc, ack2, ack2_enc, done_enc);
	dec: gen_decrypted port map(clk_in, reset_dec, enable_dec, inp_direction, direc_data, done_dec);
	timeout_mechanism : timeout port map(reset_TO, enable_TO, done_TO, clk_in);
	--direction_out : direction_data port map(clk_in, reset_direction, enable_direction, direc_data, output_result, done_direction);
	board_out : board_data port map(clk_in, reset_board, enable_board, direc_data, board_input, board_output, done_board);
	encr : encrypter port map(clk_in, K, fpga_in, fpga_enc, reset_encrypter, enable_encrypter, done_encrypter);
	transmit : uart_tx port map(clk_in, r_TX_DV, UART_share, tx_active, tx_out, w_TX_DONE);
	receive : uart_rx port map(clk_in, rx_in, w_RX_DV, UART_rec);

	process(clk_in, reset)
	begin
		if (reset = '1') then
			reset <= '0';

			Mstate <= "001";
 			state <= (others => '0');
			inp_reg <= (others => '0');
			out_reg <= (others => '0');

			sec_counter <= (others => '0');
			led_counter <= (others => '0');
			wait_counter <= (others => '0');

			reset_enc <= '1';
			--
			input_count <= (others => '0');
			trans_state <= '0';

			--
			coordinates_receive <= (others => '0');
			ack2_received <= (others => '0');

			reset_encrypter <= '1';
			fpga_in <= (others => '0');

			reset_dec <= '1';
			inp_direction <= (others => '0');
			reset_TO <= '1';

			reset_direction <= '1';

			reset_board <= '1';
			board_input <= (others => '0');
			reset_transmission <= '1';
			output_count <= (others => '0');

			reset_reception <= '1';
			display_state <= "000";
			f2hValid_out <= '0';

			uppressed <= '0';
			downpressed <= '0';
			leftpressed <= '0';
			rightpressed <= '0';

			data_output <= board_output;
			--UART_received <= (others => '1');
			uart_data_received <= '0';
			
		elsif (clk_in'event and clk_in = '1' and enable = '1') then

			if (w_RX_DV = '1') then
				uart_data_received <= '1';
			end if;
			if (reset_t = '1') then
				uart_data_received <= '0';
				uart_data_to_be_used <= '0';
			end if;
			if (upcheck = '1') then
				uppressed <= '1';
			end if ;
			if (downcheck = '1') then
				downpressed <= '1';
			end if ;
			if (leftcheck = '1') then
				leftpressed <= '1';
			end if ;
			if (rightcheck = '1') then
				rightpressed <= '1';
			end if ;
			if (resetcheck = '1') then
				reset <= '1';
			end if ;

			if (Mstate = "001") then
				if (display_state = "011") then
					Mstate <= "010";
					L <= "00000000";
					sec_counter <= (others => '0');
					display_state <= (others => '0');
				else
					if (sec_counter = "101111101011110000100000000") then
						display_state <= display_state + 1;
						sec_counter <= (others => '0');
					else
						sec_counter <= sec_counter + 1;
					end if;
					L <= (others => '1');
				end if;
			end if;

			if (Mstate = "010") then
				----------------------------- encrypt ----------------------
				-- reset
				if (state = "00000") then
					L <= "01100110";
					reset_enc <= '1';
					state <= "00001";
				end if;

				-- encrypt
				if (state = "00001") then
					if (done_enc = '1') then
						state <= "00010";
						reset_transmission <= '1';
						reset_reception <= '1';
						reset_TO <= '0';
					else
						reset_enc <= '0';
					end if;
				end if;
				------------------------------------------------------------

				----------------------------- decrypt ----------------------
				-- decrypt coordinates reset
				if (state = "01110") then
					if (reset_dec = '1') then
						reset_dec <= '0';
					else
						if (done_dec = '1') then
							state <= "01111";
						else
							reset_dec <= '0';
						end if;
					end if;
				end if;
				------------------------------------------------------------

				------------------------ transmission ----------------------
				-- transmit coordinates
				if (state = "00010") then
					if (done_TO = '1') then
						reset <= '1';
					end if;
					if (reset_transmission = '1') then
						output_count <= (others => '0');
						out_reg <= (others => '0');
						reset_transmission <= '0';
						f2hValid_out <= '0';
					elsif (output_count = "100") then
						state <= "00011";
						recep_state <= '0';
						f2hValid_out <= '0';
						reset_reception <= '1';
						output_count <= (others => '0');
					else
						if (f2hReady_in = '1') then
							if (trans_state = '0') then
								out_reg <= coordinates_enc((31-to_integer(unsigned(output_count))*8) downto (24-to_integer(unsigned(output_count))*8));
								f2hValid_out <= '1';
								trans_state <= '1';
							elsif (trans_state = '1') then
								output_count <= output_count + 1;
								trans_state <= '0';
								f2hValid_out <= '0';
							end if;
						end if;
					end if;
				end if;

				-- transmit ack1 - 1
				if (state = "00101") then
					if (done_TO = '1') then
						reset <= '1';
					end if;
					if (reset_transmission = '1') then
						output_count <= (others => '0');
						out_reg <= (others => '0');
						reset_transmission <= '0';
						f2hValid_out <= '0';
					elsif (output_count = "100") then
						state <= "00110";
						f2hValid_out <= '0';
						reset_reception <= '1';
					else
						if (f2hReady_in = '1') then
							if (trans_state = '0') then
								out_reg <= ack1_enc((31-to_integer(unsigned(output_count))*8) downto (24-to_integer(unsigned(output_count))*8));
								f2hValid_out <= '1';
								trans_state <= '1';
							elsif (trans_state = '1') then
								output_count <= output_count + 1;
								trans_state <= '0';
								f2hValid_out <= '0';
							end if;
						end if;
					end if;
				end if;

				-- transmit ack1 - 2
				if (state = "01001") then
					if (done_TO = '1') then
						reset <= '1';
					end if;
					if (reset_transmission = '1') then
						output_count <= (others => '0');
						out_reg <= (others => '0');
						reset_transmission <= '0';
						f2hValid_out <= '0';
					elsif (output_count = "100") then
						state <= "01010";
						f2hValid_out <= '0';
						reset_reception <= '1';
					else
						if (f2hReady_in = '1') then
							if (trans_state = '0') then
								out_reg <= ack1_enc((31-to_integer(unsigned(output_count))*8) downto (24-to_integer(unsigned(output_count))*8));
								f2hValid_out <= '1';
								trans_state <= '1';
							elsif (trans_state = '1') then
								output_count <= output_count + 1;
								trans_state <= '0';
								f2hValid_out <= '0';
							end if;
						end if;
					end if;
				end if;

				-- transmit ack1 - 3
				if (state = "01011") then
					if (done_TO = '1') then
						reset <= '1';
					end if;
					if (reset_transmission = '1') then
						output_count <= (others => '0');
						out_reg <= (others => '0');
						reset_transmission <= '0';
						f2hValid_out <= '0';
					elsif (output_count = "100") then
						state <= "01100";
						f2hValid_out <= '0';
						reset_transmission <= '1';
					else
						if (f2hReady_in = '1') then
							if (trans_state = '0') then
								out_reg <= ack1_enc((31-to_integer(unsigned(output_count))*8) downto (24-to_integer(unsigned(output_count))*8));
								f2hValid_out <= '1';
								trans_state <= '1';
							elsif (trans_state = '1') then
								output_count <= output_count + 1;
								trans_state <= '0';
								f2hValid_out <= '0';
							end if;
						end if;
					end if;
				end if;
				------------------------------------------------------------

				------------------------ reception -------------------------
				-- receive back the encrypted coordinates
				if (state = "00011") then
					if (done_TO = '1') then
						reset <= '1';
					end if;
					if (reset_reception = '1') then
						input_count <= (others => '0');
						reset_reception <= '0';
						--led_out <= "01100110";
					elsif (input_count = "100") then
						state <= "00100";
						--led_out <= "10011001";
					else
						if (h2fValid_in = '1' and chanAddr_in = channel_in) then
							coordinates_receive((31-to_integer(unsigned(input_count))*8) downto (24-to_integer(unsigned(input_count))*8)) <= h2fData_in;
							input_count <= input_count + 1;
						end if;
					end if;
				end if;

				-- receive back the encrypted ack2 - 1
				if (state = "00110") then
					if (done_TO = '1') then
						reset <= '1';
					end if;
					if (reset_reception = '1') then
						input_count <= (others => '0');
						reset_reception <= '0';
					elsif (input_count = "100") then
						state <= "00111";
					else
						if (h2fValid_in = '1' and chanAddr_in = channel_in) then
							ack2_received((31-to_integer(unsigned(input_count))*8) downto (24-to_integer(unsigned(input_count))*8)) <= h2fData_in;
							input_count <= input_count + 1;
						end if;
					end if;
				end if;

				-- receive the direction data1
				if (state = "01000") then
					if (done_TO = '1') then
						reset <= '1';
					end if;
					if (reset_reception = '1') then
						input_count <= (others => '0');
						reset_reception <= '0';
					elsif (input_count = "100") then
						state <= "01001";
						reset_transmission <= '1';
					else
						if (h2fValid_in = '1' and chanAddr_in = channel_in) then
							inp_direction((63-to_integer(unsigned(input_count))*8) downto (56-to_integer(unsigned(input_count))*8)) <= h2fData_in;
							input_count <= input_count + 1;
						end if;
					end if;
				end if;

				-- receive the direction data2
				if (state = "01010") then
					if (done_TO = '1') then
						reset <= '1';
					end if;
					if (reset_reception = '1') then
						input_count <= (others => '0');
						reset_reception <= '0';
					elsif (input_count = "100") then
						state <= "01011";
						reset_transmission <= '1';
					else
						if (h2fValid_in = '1' and chanAddr_in = channel_in) then
							inp_direction((31-to_integer(unsigned(input_count))*8) downto (24-to_integer(unsigned(input_count))*8)) <= h2fData_in;
							input_count <= input_count + 1;
						end if;
					end if;
				end if;

				-- receive back the encrypted ack2 - 3
				if (state = "01100") then
					if (done_TO = '1') then
						reset <= '1';
					end if;
					if (reset_reception = '1') then
						input_count <= (others => '0');
						reset_reception <= '0';
					elsif (input_count = "100") then
						state <= "01101";
						input_count <= (others => '0');
					else
						if (h2fValid_in = '1' and chanAddr_in = channel_in) then
							ack2_received((31-to_integer(unsigned(input_count))*8) downto (24-to_integer(unsigned(input_count))*8)) <= h2fData_in;
							input_count <= input_count + 1;
						end if;
					end if;
				end if;
				------------------------------------------------------------

				------------------------- check ----------------------------
				-- check received coordinates
				if (state = "00100") then
					if (done_TO = '1') then
						reset <= '1';
					end if;
					if (coordinates_receive = coordinates_enc) then
						state <= "00101";
						reset_TO <= '1';
						reset_transmission <= '1';
					end if;
				end if;

				-- check received ack2
				if (state = "00111") then
					if (done_TO = '1') then
						reset <= '1';
					end if;
					if (ack2_enc = ack2_received) then
						state <= "01000";
						reset_TO <= '1';
						reset_reception <= '1';
					end if;
				end if;

				-- again check received ack2
				if (state = "01101") then
					if (done_TO = '1') then
						state <= "00000";
					end if;
					if (ack2_enc = ack2_received) then
						state <= "01110";
						reset_TO <= '1';
					else
						reset_TO <= '0';
					end if;
				end if;
				------------------------------------------------------------

				if (state = "01111") then
					board_input(0) <= sw_in(7);
					board_input(1) <= sw_in(6);
					board_input(2) <= sw_in(5);
					board_input(3) <= sw_in(4);
					board_input(4) <= sw_in(3);
					board_input(5) <= sw_in(2);
					board_input(6) <= sw_in(1);
					board_input(7) <= sw_in(0);
					state <= "10000";
					reset_board <= '1';
				end if;

				-- generate result accoring to the board data
				if (state = "10000") then
					if (reset_board = '1') then
						reset_board <= '0';
						sec_counter <= (others => '0');
						wait_counter <= (others => '0');
						display_state <= (others => '0');
						led_counter <= (others => '0');
					elsif (done_board = '1') then
						state <= "10001";
					end if;
				end if;

				-- display result accoring to the board data
				if (state = "10001") then
					if (led_counter = "01000") then
						Mstate <= "011";
						state <= (others => '0');
						display_state <= (others => '0');
						led_counter <= (others => '0');
						sec_counter <= (others => '0');
						wait_counter <= (others => '0');
						L <= (others => '0');
					end if;
					if (sec_counter = "101111101011110000100000000") then
						display_state <= "000";
						sec_counter <= (others => '0');
					else
						sec_counter <= sec_counter + 1;
					end if;
					if (display_state = "000") then
						L <= board_output(191-(to_integer(unsigned(led_counter))*3 + to_integer(unsigned(wait_counter)))*8 downto 184-(to_integer(unsigned(led_counter))*3 + to_integer(unsigned(wait_counter)))*8);
						if (wait_counter = "10") then
							display_state <= "001";
						else
							display_state <= "010";
						end if ;
					elsif (display_state = "001") then
						wait_counter <= (others => '0');
						led_counter <= led_counter + 1;
						display_state <= "011";
					elsif (display_state = "010") then
						wait_counter <= wait_counter + 1;
						display_state <= "011";
					end if ;
				end if;
			end if;

			if (Mstate = "011") then
				if (uppressed = '1') then
					if (state = "00000") then
						if (reset_transmission = '1') then
							L <= "00000001";
							output_count <= (others => '0');
							out_reg <= (others => '0');
							reset_transmission <= '0';
							f2hValid_out <= '0';
						elsif (output_count = "100") then
							L <= "00000011";
							state <= "00001";
							f2hValid_out <= '0';
							reset_transmission <= '1';
						else
							L <= "00000010";
							if (f2hReady_in = '1') then
								if (trans_state = '0') then
									out_reg <= ack1_enc((31-to_integer(unsigned(output_count))*8) downto (24-to_integer(unsigned(output_count))*8));
									f2hValid_out <= '1';
									trans_state <= '1';
								elsif (trans_state = '1') then
									output_count <= output_count + 1;
									trans_state <= '0';
									f2hValid_out <= '0';
								end if;
							end if;
						end if;
					end if;
					if (state <= "00001") then
						if (downpressed = '1') then
							L <= "00001111";
							fpga_in(7 downto 0) <= sw_in;
							reset_encrypter <= '1';
							state <= "00010";
						else
							L <= "01000010";
						end if ;
					end if ;
					if (state = "00010") then
						if (done_encrypter = '1') then
							state <= "00011";
						else
							reset_encrypter <= '0';
						end if;
					end if;
					if (state = "00011") then
						if (reset_transmission = '1') then
							output_count <= (others => '0');
							out_reg <= (others => '0');
							reset_transmission <= '0';
							f2hValid_out <= '0';
						elsif (output_count = "100") then
							state <= "00000";
							Mstate <= "100";
							--uppressed <= '0';
							--downpressed <= '0';
							f2hValid_out <= '0';
							reset_transmission <= '1';
						else
							if (f2hReady_in = '1') then
								if (trans_state = '0') then
									out_reg <= fpga_enc((31-to_integer(unsigned(output_count))*8) downto (24-to_integer(unsigned(output_count))*8));
									f2hValid_out <= '1';
									trans_state <= '1';
								elsif (trans_state = '1') then
									output_count <= output_count + 1;
									trans_state <= '0';
									f2hValid_out <= '0';
								end if;
							end if;
						end if;
					end if ;
				else
					Mstate <= "100";
				end if ;
			end if ;

			if (Mstate = "100") then
				if (leftpressed = '1') then
					if (state = "00000") then
						if (rightpressed = '1') then
							UART_share <= sw_in;
							state <= "00010";
							r_TX_DV <= '1';
							--L <= UART_rec;
						end if ;
					end if ;
					if (state = "00010") then
						-- UART TRANSMIT
						if (w_TX_DONE = '1') then
						--L <= UART_rec;
						--UART_WRITE_BYTE(UART_share, r_RX_SERIAL);
						--L <= UART_temp;
							r_TX_DV <= '0';
							L <= "11001100";
							Mstate <= "101";
							leftpressed <= '0';
							rightpressed <= '0';
							state <= "00000";
							reset_transmission <= '1';
						else
							L <= "10100101";
						end if;
					end if ;
				else
					Mstate <= "101";
				end if ;
			end if ;

			if (Mstate = "101") then
				--L <= UART_rec;
				-- RECEIVE FROM UART
				--Mstate <= "110";
				if (state = "00000") then
					if (w_RX_DV = '0') then
						state <= "00001";
						uart_data_received <= '0';
					else
						Mstate <= "110";
					end if;
				elsif (state = "00001") then
					UART_received <= UART_rec;
					uart_data_to_be_used <= '1';
					state <= "00000";
					Mstate <= "110";
					L <= UART_rec;
				end if ;
			end if ;

			if (Mstate = "110") then
				--L <= "11111110";
				if (led_counter = "01010") then
					Mstate <= "010";
					led_counter <= (others => '0');
				end if;
				if (sec_counter = "101111101011110000100000000") then
					led_counter <= led_counter + 1;
					sec_counter <= (others => '0');
				else
					sec_counter <= sec_counter + 1;
				end if;
			end if;
		




		end if;
	end process;
			
	led_out <= L;

	checksum_next <=
		std_logic_vector(unsigned(checksum) + unsigned(h2fData_in))
			when chanAddr_in = "0000000" and h2fValid_in = '1'
		else h2fData_in & checksum(7 downto 0)
			when chanAddr_in = "0000001" and h2fValid_in = '1'
		else checksum(15 downto 8) & h2fData_in
			when chanAddr_in = "0000010" and h2fValid_in = '1'
		else checksum;

	-- when the host is reading
	with chanAddr_in select f2hData_out <=
		out_reg when channel_out,
		x"00" when others;

	h2fReady_out <= '1'; 

	flags <= "00" & f2hReady_in & reset_in;
	seven_seg : entity work.seven_seg
		port map(
			clk_in     => clk_in,
			data_in    => checksum,
			dots_in    => flags,
			segs_out   => sseg_out,
			anodes_out => anode_out
		);
end architecture;
