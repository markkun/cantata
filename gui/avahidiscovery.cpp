/*
 *Copyright (C) <2017>  Alex B
 *
 *This program is free software: you can redistribute it and/or modify
 *it under the terms of the GNU General Public License as published by
 *the Free Software Foundation, either version 2 of the License, or
 *(at your option) any later version.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *
 *You should have received a copy of the GNU General Public License
 *along with this program.  If not, see <http://www.gnu.org/licenses/>
 */


#include "avahidiscovery.h"

#include <avahi-common/error.h>

#include <QDebug>

const AvahiPoll* getAvahiPoll(void);


AvahiDiscovery *AvahiDiscovery::ptr = nullptr;

void AvahiDiscovery::resolver_callback(
    AvahiServiceResolver *r,
    AVAHI_GCC_UNUSED AvahiIfIndex interface,
    AVAHI_GCC_UNUSED AvahiProtocol protocol,
    AvahiResolverEvent event,
    const char *name,
    AVAHI_GCC_UNUSED const char *type,
    AVAHI_GCC_UNUSED const char *domain,
    AVAHI_GCC_UNUSED const char *host_name,
    const AvahiAddress *address,
    uint16_t port,
    AVAHI_GCC_UNUSED AvahiStringList *txt,
    AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
    void *userdata)
{
    char a[AVAHI_ADDRESS_STR_MAX];

    switch(event)
    {
        case AVAHI_RESOLVER_FAILURE:
            //avahi resolver not found
            break;
        case AVAHI_RESOLVER_FOUND:
            avahi_address_snprint(a, sizeof(a), address);
            break;
    }

    int eventType = *( static_cast<int*>(userdata) );
    switch(eventType)
    {
        case AVAHI_BROWSER_NEW:
            emit ptr->mpdFound(name, a, port);
            break;
        case AVAHI_BROWSER_REMOVE:
            emit ptr->mpdRemoved(name);
            break;
    }

    avahi_service_resolver_free(r);
    free(userdata);
}

void AvahiDiscovery::browse_callback(
    AvahiServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
    AVAHI_GCC_UNUSED void* userdata)
{
    switch (event)
    {
        case AVAHI_BROWSER_FAILURE:
            fprintf(stderr, "(Browser) %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
            break;
        case AVAHI_BROWSER_NEW:
            if (!(avahi_service_resolver_new (avahi_service_browser_get_client(b), interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, AVAHI_LOOKUP_USE_MULTICAST, resolver_callback, new int(AVAHI_BROWSER_NEW))))
                fprintf(stderr, "Failed to resolve service '%s'\n", name);
            break;
        case AVAHI_BROWSER_REMOVE:
            if (!(avahi_service_resolver_new (avahi_service_browser_get_client(b), interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, AVAHI_LOOKUP_USE_MULTICAST, resolver_callback, new int(AVAHI_BROWSER_REMOVE))))
                fprintf(stderr, "Failed to resolve service '%s'\n", name);
             break;
         case AVAHI_BROWSER_ALL_FOR_NOW:
             //fprintf(stderr, "Service browser: ALL_FOR_NOW\n");
             break;
         case AVAHI_BROWSER_CACHE_EXHAUSTED:
             //fprintf(stderr, "Service browser: CACHE_EXHAUSTED\n");
             break;
    }
}


void AvahiDiscovery::client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void *userdata)
 {
    assert(c);

    if (state == AVAHI_CLIENT_FAILURE)
    {
        fprintf(stderr, "Server connection failure: %s\n", avahi_strerror(avahi_client_errno(c)));
    }
 }


AvahiDiscovery::AvahiDiscovery()
{
    ptr = this;

    /* create avahi client */
    int error;
    m_client = avahi_client_new(getAvahiPoll(), AVAHI_CLIENT_IGNORE_USER_CONFIG, client_callback, NULL, &error);
    if (!m_client)
    {
        fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));
        return;
    }

    /* create avahi browser */
    m_browser = avahi_service_browser_new(m_client, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, "_mpd._tcp", NULL, AVAHI_LOOKUP_USE_MULTICAST, browse_callback, m_client);
    if (!m_browser) 
    {
        fprintf(stderr, "Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(m_client)));
        return;
    }
}

AvahiDiscovery::~AvahiDiscovery()
{
    if (m_browser)
    {
        avahi_service_browser_free(m_browser);
    }

    if (m_client)
    {
        avahi_client_free(m_client);
    }
}

