/////////////////////////////////////////////////////////////////////////////
// This file is part of the Journey MMORPG client                           //
// Copyright � 2015 Daniel Allendorf                                        //
//                                                                          //
// This program is free software: you can redistribute it and/or modify     //
// it under the terms of the GNU Affero General Public License as           //
// published by the Free Software Foundation, either version 3 of the       //
// License, or (at your option) any later version.                          //
//                                                                          //
// This program is distributed in the hope that it will be useful,          //
// but WITHOUT ANY WARRANTY; without even the implied warranty of           //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            //
// GNU Affero General Public License for more details.                      //
//                                                                          //
// You should have received a copy of the GNU Affero General Public License //
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    //
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "PacketHandler.h"
#include "UILogin.h"
#include "UILoginNotice.h"
#include "UIWorldSelect.h"
#include "UICharSelect.h"
#include "LoginPackets83.h"

namespace Net
{
	// Handler for a packet that contains the response to an attempt at logging in.
	class LoginResultHandler83 : public PacketHandler
	{
		void handle(ClientInterface& client, InPacket& recv) const
		{
			// Remove waiting information and make sure textfields are not focused anymore.
			client.getui().remove(IO::UI_LOGINWAIT);
			client.getwindow().getkeyboard().focustarget(0);

			// The packet should contain a 'reason' integer which can signify various things.
			int32_t reason = recv.readint();
			if (reason != 0)
			{
				// Login unsuccessfull. The LoginNotice displayed will contain the specific information.
				using::IO::ElementLoginNotice;
				switch (reason)
				{
				case 2:
					client.getui().add(ElementLoginNotice(16));
					return;
				case 7:
					client.getui().add(ElementLoginNotice(17));
					return;
				case 23:
					// The server sends a request to accept the terms of service. For convenience, just auto-accept.
					client.getsession().dispatch(TOSPacket83());
					return;
				default:
					// Other reasons.
					if (reason > 0)
					{
						client.getui().add(ElementLoginNotice(reason - 1));
					}
				}
				client.getui().enable();
			}
			else
			{
				// Login successfull. The packet contains information on the account, so we initialise the account with it.
				client.getlogin().parseaccount(recv);
				// Save the Login ID if the box for it on the login panel is checked.
				if (client.getconfig().getbool("SaveLogin"))
				{
					client.getconfig().setstring("Account", client.getlogin().getaccount().getname());
				}
				// Request the worlds and channels online.
				client.getsession().dispatch(ServerRequestPacket83());
			}
		}
	};

	// Handles the packet that contains information on worlds and channels.
	class ServerlistHandler83 : public PacketHandler
	{
		void handle(ClientInterface& client, InPacket& recv) const
		{
			// Remove the Login UI.
			client.getui().remove(IO::UI_LOGIN);
			// Parse all worlds.
			client.getlogin().parseworld(recv);

			// Add the world selection screen to the ui.
			using::IO::ElementWorldSelect;
			client.getui().add(ElementWorldSelect(client.getui(), client.getlogin(), client.getsession()));
			client.getui().enable();
		}
	};

	// Handler for a packet that contains information on all chars on this world.
	class CharlistHandler83 : public PacketHandler
	{
		void handle(ClientInterface& client, InPacket& recv) const
		{
			recv.skip(1);

			// Parse all characters.
			size_t numchars = recv.readbyte();
			for (size_t i = 0; i < numchars; i++)
			{
				client.getlogin().getaccount().parsecharentry(recv);
			}

			// Additional information for login: Wether to use a pic and how many characters can be created.
			client.getlogin().getaccount().setpic(recv.readbyte());
			client.getlogin().getaccount().setslots(recv.readint());

			// Remove the world selection screen.
			client.getui().remove(IO::UI_WORLDSELECT);

			// Add the character selection screen.
			using::IO::ElementCharSelect;
			client.getui().add(ElementCharSelect(client.getcache().getequips(), client.getui(), client.getlogin(), client.getsession()));
			client.getui().enable();
		}
	};

	// Handler for a packet which responds to the request for a character name.
	class CharnameResultHandler83 : public PacketHandler
	{
		void handle(ClientInterface& client, InPacket& recv) const
		{
			// Read the name and if it is already in use.
			string name = recv.readascii();
			bool used = recv.readbool();

			using::IO::UIElement;
			UIElement& uicc = client.getui().getelement(IO::UI_CHARCREATION);
			if (used)
			{
				using::IO::ElementLoginNotice;
				client.getui().add(ElementLoginNotice(5));
			}
			else
			{

			}
			client.getui().enable();
		}
	};

	// Handler for the packet that notifies that a char was successfully created.
	class AddNewcharHandler83 : public PacketHandler
	{
		void handle(ClientInterface& client, InPacket& recv) const
		{
			// Some check.
			int8_t stuff = recv.readbyte();
			if (stuff == 0)
			{
				// Parse info on the new character.
				client.getlogin().getaccount().parsecharentry(recv);
				// Remove the character creation ui.
				client.getui().getelement(IO::UI_CHARCREATION).deactivate();
				client.getui().remove(IO::UI_CHARCREATION);

				// Readd the updated character selection.
				using::IO::ElementCharSelect;
				client.getui().add(ElementCharSelect(client.getcache().getequips(), client.getui(), client.getlogin(), client.getsession()));
				client.getui().enable();
			}
		}
	};

	// Handler for a packet that responds to the request to the delete a character.
	class DeleteCharResultHandler83 : public PacketHandler
	{
		void handle(ClientInterface& client, InPacket& recv) const
		{
			// Read the character id and if deletion was successfull (pic was correct).
			int cid = recv.readint();
			bool success = recv.readbool();

			// Show the result to the user.
			using::IO::ElementLoginNotice;
			if (success)
			{
				client.getui().add(ElementLoginNotice(55));
			}
			else
			{
				client.getui().add(ElementLoginNotice(93));
			}
		}
	};

	// Handles the packet which contains the IP of a channel server to connect to.
	class ServerIPHandler83 : public PacketHandler
	{
		void handle(ClientInterface& client, InPacket& recv) const
		{
			recv.skip(2);

			// Read the ipv4 adress in a string.
			string addrstr;
			for (int i = 0; i < 4; i++)
			{
				uint8_t num = static_cast<uint8_t>(recv.readbyte());
				addrstr.append(std::to_string(num));
				if (i < 3)
				{
					addrstr.push_back('.');
				}
			}

			// Read the port adress in a string.
			string portstr = std::to_string(recv.readshort());
			
			// Attempt to reconnect to the server and if successfull, login to the game.
			int32_t cid = recv.readint();
			bool connected = client.getsession().reconnect(addrstr.c_str(), portstr.c_str());
			if (connected)
			{
				client.getsession().dispatch(PlayerLoginPacket83(cid));
			}
			else
			{
				client.getsession().disconnect();
			}
		}
	};
}