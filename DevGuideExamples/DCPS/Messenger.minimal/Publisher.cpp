/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Boilerplate.h"
#include <dds/DCPS/Service_Participant.h>
#include <model/Sync.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {
    // Initialize DomainParticipantFactory, handling command line args
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    // Create domain participant
    DDS::DomainParticipant_var participant = createParticipant(dpf);

    // Register type support and create topic
    DDS::Topic_var topic = createTopic(participant);

    // Create publisher
    DDS::Publisher_var publisher = createPublisher(participant);

    // Create data writer for the topic
    DDS::DataWriter_var writer = createDataWriter(publisher, topic);

    // Safely downcast data writer to type-specific data writer
    Messenger::MessageDataWriter_var message_writer = narrowWriter(writer);

    {
      // Block until Subscriber is available
      OpenDDS::Model::WriterSync ws(writer);

      // Write samples
      Messenger::Message message;
      message.subject_id = 99;

      message.from       = "Comic Book Guy";
      message.subject    = "Review";
      message.text       = "Worst. Movie. Ever.";
      message.count      = 0;

      int max_msgs = 10;
      if (argc > 1) {
        max_msgs = atoi(argv[1]);
      }

      for (int i = 0; i < max_msgs; ++i) {
        // Publish the message
        DDS::ReturnCode_t error = message_writer->write(message,
                                                        DDS::HANDLE_NIL);
        if (error != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("ERROR: %N:%l: main() -")
                     ACE_TEXT(" write returned %d!\n"), error));
        }

        // Prepare next sample
        ++message.count;
        ++message.subject_id;
      }

      // End of WriterSyncScope - block until messages acknowedged
    }

    // Clean-up!
    cleanup(participant, dpf);

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  } catch (std::string& msg) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"),
                      msg.c_str()), -1);
  }

  return 0;
}
