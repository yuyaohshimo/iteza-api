class ReceiveJob
  def receive(message, params)
    if message.multipart? && message.has_attachments?
      message.attachments.each do |a|
        if ["image/gif", "image/jpeg", "image/png"].include? a.mime_type
          puts "添付ファイルみっけ。 #{a.filename}"
        end
      end
    end
  end
end