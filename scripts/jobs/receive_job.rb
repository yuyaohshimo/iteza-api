require 'tempfile'
require 'rest-client'
require 'json'

require File.expand_path(File.dirname(__FILE__) + '/../modules/send_mail')

class ReceiveJob
  include SendMail

  TMP_DIR = "tmp"
  TMP_FILE_PREFIX = "checky_"
  TMP_FILE_EXTENSIONS = {
    "image/jpeg" => ".jpg",
    "image/png"  => ".png",
  }

  SCRIPT_PATH = "scripts/get_facevalue.sh"

  API_URL = "http://www.fiftyriver.net:3000/api/check/receive"

  def receive(message, params)
    attachment = nil
    mime_type = ""

    if message.multipart? && message.has_attachments?
      message.attachments.each do |a|
        mime_type = a.mime_type
        if ["image/jpeg", "image/png"].include? mime_type
          attachment = a.body.decoded
          break
        end
      end
    else
      attachment = message.body.decoded
      mime_type  = message.mime_type
    end

    puts "添付ファイル？ #{mime_type}"

    f = Tempfile.new([TMP_FILE_PREFIX, TMP_FILE_EXTENSIONS[mime_type]], TMP_DIR)
    f.write attachment

    puts "添付ファイルみっけ。 #{f.path}"

    r = `#{SCRIPT_PATH} #{File.absolute_path f}`
    puts r

    receiver = message.from_addrs[0]

    if ($? == 0)
      id = r.chomp

      begin
        response = RestClient.post API_URL,
          { id: id, receiver: receiver }.to_json,
          content_type: :json,
          accept: :json

        puts JSON.parse(response)

        send_receive_success(receiver)
      rescue => e
        send_receive_wrong_check(receiver)
        puts e.message
      end
    else
      send_receive_failure(receiver)
    end
  end
end