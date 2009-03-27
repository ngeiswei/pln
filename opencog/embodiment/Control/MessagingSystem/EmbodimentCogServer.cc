/*
 * opencog/embodiment/Control/EmbodimentCogServer.cc
 *
 * Copyright (C) 2008 by Singularity Institute for Artificial Intelligence
 * All Rights Reserved
 *
 * Written by Welter Luigi <welter@vettalabs.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "EmbodimentCogServer.h"

#include <opencog/util/Logger.h>
#include <opencog/util/Config.h>

using namespace opencog;
using namespace MessagingSystem;

EmbodimentCogServer::EmbodimentCogServer()
{
    logger().info("[EmbodimentCogServer] constructor");
}

EmbodimentCogServer::~EmbodimentCogServer()
{
    logger().info("[EmbodimentCogServer] destructor");
}

void EmbodimentCogServer::setNetworkElement(NetworkElement* _ne) {
    ne = _ne;
    externalTickMode = config().get_bool("EXTERNAL_TICK_MODE");
    unreadMessagesCheckInterval = atoi(ne->parameters.get("UNREAD_MESSAGES_CHECK_INTERVAL").c_str()); 
    unreadMessagesRetrievalLimit = atoi(ne->parameters.get("UNREAD_MESSAGES_RETRIEVAL_LIMIT").c_str()); 
}

NetworkElement& EmbodimentCogServer::getNetworkElement(void) {
    return *ne;
}

bool EmbodimentCogServer::customLoopRun(void)
{
    unsigned int msgQueueSize = ne->getIncomingQueueSize();
    if (msgQueueSize > 0) {
        logger().debug("EmbodimentCogServer(%s) - messageCentral size: %d", 
                ne->getID().c_str(), msgQueueSize);
    }


    if (!externalTickMode) {
        // Retrieve new messages from router, if any
        if ((cycleCount % unreadMessagesCheckInterval) == 0) {
            ne->retrieveMessages(unreadMessagesRetrievalLimit);
        }

        // Process all messages in queue
        std::vector<Message*> messages; 
        while (!ne->isIncomingQueueEmpty() ) {
            Message *message = ne->popIncomingQueue();
            messages.push_back(message);
        }
        for(unsigned int i = 0; i < messages.size(); i++) {
            Message *message = messages[i];
            if (message->getType() != Message::TICK) {
                running = !processNextMessage(message);
                delete(message);
                if (!running) return false;
            } else {
                delete(message);
            }
        }
        return true;
    } else {
        ne->retrieveMessages(unreadMessagesRetrievalLimit);

        // Get all messages in the incoming queue (or until a TICK message is received)
        std::vector<Message*> messages; 
        while (!ne->isIncomingQueueEmpty()) {
            Message *message = ne->popIncomingQueue();
            messages.push_back(message);
            if (message->getType() == Message::TICK) break; 
        }
        for(unsigned int i = 0; i < messages.size(); i++) {
            Message *message = messages[i];
            //check type of message
            if (message->getType() == Message::TICK) {
                logger().log(opencog::Logger::DEBUG, "EmbodimentCogServer - got a tick");
                delete(message);
                return true; 
            } else {
                running = !processNextMessage(message);
                delete(message);
                if (!running) return false;
            }
        }
    }
    
    //sleep a bit to not keep cpu busy and do not keep requesting for unread messages 
    usleep(50000);

    return false;
}

bool EmbodimentCogServer::sendMessage(Message &msg) {
    return ne->sendMessage(msg);
}

bool EmbodimentCogServer::sendCommandToRouter(const std::string &cmd) {
    return ne->sendCommandToRouter(cmd);
}

Control::SystemParameters& EmbodimentCogServer::getParameters() {
    return ne->parameters;
}

const std::string& EmbodimentCogServer::getID() {
    return ne->getID();
}

int EmbodimentCogServer::getPortNumber() {
    return ne->getPortNumber();
}

bool EmbodimentCogServer::isElementAvailable(const std::string& id) {
    return ne->isElementAvailable(id);
}

void EmbodimentCogServer::logoutFromRouter() {
    return ne->logoutFromRouter();
}
