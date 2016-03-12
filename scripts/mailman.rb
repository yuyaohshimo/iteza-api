require 'mailman'

Dir[File.expand_path('../jobs', __FILE__) << '/*.rb'].each do |file|
  require file
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
      else
        nil
      end
    end
  end
end

Mailman::Application.run do
  attachment true, ReceiveJob # do
#    puts 'Hello!'
#  end

  body '入金', DepositJob # do
#    puts '入金しますよ？'
#  end

  body /[0-9]{10}/, ConfirmJob # do
#    puts '入金したよ'
#  end
end