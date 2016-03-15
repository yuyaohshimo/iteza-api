require 'mailman'

$LOAD_PATH << File.dirname(__FILE__)

['../jobs'].each do |p|
  Dir[File.expand_path(p, __FILE__) << '/*.rb'].each do |file|
    require file
  end
end

Mail.defaults do
  delivery_method :smtp, {
    address:   'smtp.muumuu-mail.com',
    port:      465,
    domain:    'checky.me',
    user_name: 'get@checky.me',
    password:  ENV['CHECKY_PASSWORD'],
    authentication: 'plain',
    ssl: true
  }
end

Mailman.config.imap = {
  server: 'imap4.muumuu-mail.com',
  port: 993,
  ssl: true,
  username: 'get@checky.me',
  password: ENV['CHECKY_PASSWORD']
}

Mailman.config.maildir = "cache/mail"
Mailman.config.poll_interval = 10

class Mailman::Route
  class AttachmentCondition < Condition
    def match(message)
      if message.multipart? && message.has_attachments?
        [{}, []]
      elsif message.attachment?
        [{}, []]
      else
        nil
      end
    end
  end
end

class Mailman::Route
  # Matches against the Body of a message.
  class BodyCondition < Condition
    def match(message)
      if message.multipart?
        result = nil
        message.parts.each do |part|
          break if result = @matcher.match(part.decoded)
        end
        return result
      else
        @matcher.match(message.body.decoded.force_encoding('utf-8'))
      end
    end
  end
end

Mailman::Application.run do
  attachment true, ReceiveJob # do
#    puts 'Hello!'
#  end

  body '入金', DepositRequestJob # do
#    puts '入金しますよ？'
#  end

  body /[0-9]{10}/, DepositJob # do
#    puts 'ここでいい？'
#  end

  body 'はい', DepositConfirmJob # do
#    puts '入金したよ！'
#  end
end